// Generates paired browser fixtures + Masharif harness cases from one shared spec tree, so
// the HTML and the C++ describe the *same* layout by construction instead of by hand-sync.
// Hand-writing both halves is fine for a 5-node fixture; for the ~100-node full-page cases
// in pages/ it guarantees drift that would read as an engine bug.
//
// Run: node generate.mjs   (CMake runs it for you -- see CMakeLists.txt in this directory)
//
// Outputs (all committed, so a reviewer sees exactly what the harness compiles):
//   fixtures/<case>.html  -- standalone browser-truth page, same reset as the hand-written ones
//   generated_cases.inc   -- Fixture_<case>(float&, float&) bodies, #included into harness.cpp
//   generated_list.inc    -- {"<case>", Fixture_<case>}, spliced into harness.cpp's fixture list
//
// Every spec is validated against the authoring rules in README.md ("Authoring rules for
// generated cases") before anything is emitted. The rules exist because the engine and CSS
// disagree on specific defaults and specific unimplemented paths; a spec that trips one gets
// a hard error naming the rule and the source line, not a silently wrong fixture. A case that
// wants to *demonstrate* a divergence opts in per node with `allowGap: ['R7']`.

import { readdirSync, writeFileSync, mkdirSync } from 'node:fs';
import { fileURLToPath, pathToFileURL } from 'node:url';
import path from 'node:path';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const FIXTURES_DIR = path.join(__dirname, 'fixtures');
const PAGES_DIR = path.join(__dirname, 'pages');

// ---------------------------------------------------------------- enum maps

const JUSTIFY = {
    'flex-start': 'FlexStart', 'flex-end': 'FlexEnd', 'center': 'FlexCenter',
    'space-between': 'SpaceBetween', 'space-around': 'SpaceAround', 'space-evenly': 'SpaceEvenly',
};
const ALIGN_ITEMS = {
    'stretch': 'Stretch', 'flex-start': 'FlexStart', 'flex-end': 'FlexEnd',
    'center': 'FlexCenter', 'baseline': 'Baseline', 'auto': 'AutoAlign',
};
const ALIGN_CONTENT = {
    'stretch': 'Stretch', 'flex-start': 'FlexStart', 'flex-end': 'FlexEnd', 'center': 'FlexCenter',
    'space-between': 'SpaceBetween', 'space-around': 'SpaceAround', 'space-evenly': 'SpaceEvenly',
};
const DIRECTION = { 'row': 'Row', 'row-reverse': 'RowReverse', 'column': 'Column', 'column-reverse': 'ColumnReverse' };
const WRAP = { 'nowrap': 'NoWrap', 'wrap': 'Wrap', 'wrap-reverse': 'WrapReverse' };
const POSITION = { 'static': 'Static', 'relative': 'Relative', 'absolute': 'Absolute' };
const DISPLAY_CPP = {
    'block': 'Block', 'flex': 'Flex', 'inline-block': 'InlineBlock',
    'inline-flex': 'InlineFlex', 'none': 'None',
};

const SIDES = ['top', 'right', 'bottom', 'left'];
const FLEX_CONTAINERS = new Set(['flex', 'inline-flex']);
const INLINE_LEVEL = new Set(['inline-block', 'inline-flex']);

// ---------------------------------------------------------------- value helpers

const isPercent = (v) => typeof v === 'string' && v.trim().endsWith('%');
const isAuto = (v) => v === 'auto' || v === null;

function fLit(n) {
    const v = Number(n);
    if (!Number.isFinite(v)) throw new Error(`not a finite number: ${n}`);
    return (Number.isInteger(v) ? v.toFixed(1) : String(v)) + 'f';
}

/// Spec value -> CSS length. `auto`/undefined become the CSS keyword, so an explicitly-auto
/// entry in a spec reads the same in both outputs.
function cssLen(v) {
    if (v === undefined || isAuto(v)) return 'auto';
    if (isPercent(v)) return v.trim();
    return `${Number(v)}px`;
}

/// Spec value -> CSSValue(...). A default-constructed CSSValue is NaN, i.e. CSSUnit::Auto,
/// which is how the engine spells `auto` (CSSValue.h:19).
function cppLen(v) {
    if (v === undefined || isAuto(v)) return 'CSSValue()';
    if (isPercent(v)) return `CSSValue(${fLit(parseFloat(v))}, CSSUnit::Percent)`;
    return `CSSValue(${fLit(v)})`;
}

const edgeTotal = (edge, ...sides) =>
    sides.reduce((sum, s) => sum + (typeof edge?.[s] === 'number' ? edge[s] : 0), 0);
const edgeHasAny = (edge) => !!edge && SIDES.some((s) => edge[s] !== undefined);

// ---------------------------------------------------------------- validation

class SpecError extends Error {}

function fail(caseName, spec, rule, why) {
    throw new SpecError(`[${caseName}] node "${spec.id}" violates ${rule}: ${why}\n` +
        `    Either fix the spec or opt in with allowGap: ['${rule}'] to assert the divergence ` +
        `on purpose (and declare it in compare.mjs's KNOWN_GAPS).`);
}

/// Enforces the authoring rules. Each one names the engine source that makes it necessary --
/// they are not style preferences, they are the boundary of what the two sides can agree on.
function validateNode(caseName, spec, ancestors, seenIds) {
    if (!spec.id) throw new SpecError(`[${caseName}] a node has no id`);
    if (seenIds.has(spec.id)) throw new SpecError(`[${caseName}] duplicate id "${spec.id}" -- ids must be unique per fixture (extract.mjs keys on them)`);
    seenIds.add(spec.id);

    const allowed = new Set(spec.allowGap ?? []);
    const skip = (rule) => allowed.has(rule);
    const display = spec.display ?? 'block';
    if (!(display in DISPLAY_CPP)) throw new SpecError(`[${caseName}] node "${spec.id}": unknown display "${display}"`);

    const children = spec.children ?? [];
    const parent = ancestors.length ? ancestors[ancestors.length - 1] : null;
    const parentDisplay = parent ? (parent.display ?? 'block') : null;
    const inFlow = !spec.position || spec.position === 'static' || spec.position === 'relative';
    const inNormalFlow = parentDisplay !== null && !FLEX_CONTAINERS.has(parentDisplay) && inFlow;

    // R9: Style::Modify<T>() accepts only {Dimensions, CSSFlex, MarginEdge, PaddingEdge,
    // BorderProperties} (Style.h:36-42). PositionOffsets -- the struct Node's Relative branch
    // reads -- is not in that list, so a relative top/left can never be written through the
    // public API and is silently a no-op.
    if (spec.position === 'relative' && spec.offsets && !skip('R9'))
        fail(caseName, spec, 'R9', 'position:relative with offsets -- PositionOffsets has no public setter (Style.h:36-42)');

    // R4: an AUTO-width inline-level box gets a hardcoded 100px placeholder instead of
    // shrink-to-fit (Node.cpp:602 "no shrink-to-fit floor yet").
    if (INLINE_LEVEL.has(display) && spec.width === undefined && !skip('R4'))
        fail(caseName, spec, 'R4', 'inline-level box needs an explicit width -- AUTO is a 100px placeholder (Node.cpp:602)');

    // R6: NormalFlowStrategy's line height adds the child's padding and border on top of
    // ComputedHeight, which already contains both (NormalFlowStrategy.cpp:107-110), and
    // LayoutLine writes LocalX before advancing by margin-left, so margin-left offsets the
    // *next* box rather than this one (NormalFlowStrategy.cpp:14-21).
    if (INLINE_LEVEL.has(display) && inNormalFlow && !skip('R6')) {
        if (edgeTotal(spec.padding, ...SIDES) !== 0 || edgeTotal(spec.border, ...SIDES) !== 0)
            fail(caseName, spec, 'R6', 'inline-level box cannot carry padding/border -- line height double-counts them (NormalFlowStrategy.cpp:107-110)');
        if (edgeTotal(spec.margin, 'left') !== 0)
            fail(caseName, spec, 'R6', 'inline-level box cannot carry margin-left -- LayoutLine applies it to the next box (NormalFlowStrategy.cpp:14-21)');
        // margin-top feeds the line height but never LocalY, so the box stays on the line's top
        // edge while the line below it moves. margin-right and margin-bottom both agree with CSS.
        if (edgeTotal(spec.margin, 'top') !== 0)
            fail(caseName, spec, 'R6', 'inline-level box cannot carry margin-top -- LayoutLine writes LocalY without it (NormalFlowStrategy.cpp:16-19)');
    }

    // R7: the block branch writes LocalY without the child's own margin-top, then advances
    // currentY by it (NormalFlowStrategy.cpp:95-99) -- so margin-top shifts the *following*
    // sibling. Using margin-bottom only also sidesteps CSS adjacent-sibling margin collapsing,
    // which the engine does not model at all.
    if (inNormalFlow && (display === 'block' || display === 'flex') && edgeTotal(spec.margin, 'top') !== 0 && !skip('R7'))
        fail(caseName, spec, 'R7', 'block-level child cannot carry margin-top -- use margin-bottom on the previous sibling (NormalFlowStrategy.cpp:95-99)');

    // R5: LayoutLine starts x at 0 rather than at the content-box origin, so a container of
    // inline-level children ignores its own padding and border for them
    // (NormalFlowStrategy.cpp:12-13).
    const hasInlineChild = children.some((c) => INLINE_LEVEL.has(c.display ?? 'block'));
    if (hasInlineChild && !FLEX_CONTAINERS.has(display) && !skip('R5')) {
        if (edgeTotal(spec.padding, ...SIDES) !== 0 || edgeTotal(spec.border, ...SIDES) !== 0)
            fail(caseName, spec, 'R5', 'container of inline-level children cannot carry padding/border -- LayoutLine starts at the border-box origin (NormalFlowStrategy.cpp:12-13)');
    }

    // R8: ApplyBlockAutoHeight adds padding-top/border-top on top of a child LocalY that the
    // block branch already offset by both (Node.cpp:447-451 vs NormalFlowStrategy.cpp:96).
    const autoHeightBlock = (display === 'block' || display === 'inline-block') &&
        spec.height === undefined && children.length > 0;
    if (autoHeightBlock && !skip('R8')) {
        if (edgeTotal(spec.padding, 'top') !== 0 || edgeTotal(spec.border, 'top') !== 0)
            fail(caseName, spec, 'R8', 'auto-height block cannot carry padding-top/border-top -- ApplyBlockAutoHeight double-counts them (Node.cpp:447-451)');
    }

    // R11: the AUTO branches clamp the *content* box with min/max and then add padding+border
    // (Node.cpp:632-640), where CSS border-box sizing clamps the border box.
    const autoWidth = spec.width === undefined;
    // `minWidth: 0` is not a clamp: max(x, 0) is x for any box size, so it cannot expose the
    // content-box/border-box difference. It appears all over the specs only because the flex
    // automatic minimum size has to be spelled out (see styleDecls).
    const hasMinMaxWidth = (spec.minWidth !== undefined && spec.minWidth !== 0) || spec.maxWidth !== undefined;
    if (autoWidth && hasMinMaxWidth && edgeTotal(spec.padding, 'left', 'right') + edgeTotal(spec.border, 'left', 'right') !== 0 && !skip('R11'))
        fail(caseName, spec, 'R11', 'auto width + horizontal padding/border + min/max-width clamp different boxes (Node.cpp:632-640)');

    // R12: FindContainingBlock walks to the nearest non-static ancestor and stops at the root
    // (Node.cpp:22-32), then PositionOutOfFlowChild anchors the child at that ancestor's
    // *content*-box origin (Node.cpp:689-697) and sizes percentage insets against its *border*
    // box (Node.cpp:216-218). CSS uses the padding box for both. The two agree exactly when the
    // containing block carries no padding (origin) and no border (inset reference) -- so wrap a
    // padded box in an unpadded positioned parent rather than positioning the padded box itself.
    if (spec.position === 'absolute' && !skip('R12')) {
        const cb = [...ancestors].reverse().find((a) => a.position === 'relative' || a.position === 'absolute');
        if (!cb) {
            fail(caseName, spec, 'R12', 'absolute box has no positioned ancestor -- the browser would use the viewport as containing block, the engine falls back to root (Node.cpp:22-32)');
        } else if (edgeTotal(cb.padding, ...SIDES) !== 0 || edgeTotal(cb.border, ...SIDES) !== 0) {
            fail(caseName, spec, 'R12', `containing block "${cb.id}" must have no padding/border -- the engine anchors at its content-box origin and measures insets against its border box (Node.cpp:689-697, 216-218)`);
        }
    }

    for (const child of children) validateNode(caseName, child, [...ancestors, spec], seenIds);
}

function validateCase(testCase) {
    if (!testCase.name) throw new SpecError('a case has no name');
    if (!/^[a-z][a-z0-9_]*$/.test(testCase.name))
        throw new SpecError(`case "${testCase.name}": name must be snake_case (it becomes a C++ identifier and a filename)`);
    if (!testCase.root) throw new SpecError(`case "${testCase.name}" has no root`);
    if (testCase.root.id !== 'root') throw new SpecError(`case "${testCase.name}": root node's id must be "root" (extract.mjs measures relative to it)`);
    validateNode(testCase.name, testCase.root, [], new Set());
}

// ---------------------------------------------------------------- HTML emission

/// Emits only the declarations that actually differ from the CSS initial value, plus the two
/// that must always be stated because the engine's initial value differs:
///   min-width/min-height -- Dimensions defaults both to 0 (Dimension.h:11-12), while CSS
///   resolves `auto` on a flex item to a content-based floor. Left implicit, every flex item
///   in a shrinking container would disagree for a reason that has nothing to do with layout.
function styleDecls(spec, parent) {
    const d = [];
    const display = spec.display ?? 'block';
    d.push(`display:${display}`);

    if (FLEX_CONTAINERS.has(display)) {
        if (spec.direction) d.push(`flex-direction:${spec.direction}`);
        if (spec.wrap) d.push(`flex-wrap:${spec.wrap}`);
        if (spec.justify) d.push(`justify-content:${spec.justify}`);
        if (spec.alignItems) d.push(`align-items:${spec.alignItems}`);
        if (spec.alignContent) d.push(`align-content:${spec.alignContent}`);
        if (spec.gapRow !== undefined) d.push(`row-gap:${cssLen(spec.gapRow)}`);
        if (spec.gapColumn !== undefined) d.push(`column-gap:${cssLen(spec.gapColumn)}`);
    }

    if (spec.width !== undefined) d.push(`width:${cssLen(spec.width)}`);
    if (spec.height !== undefined) d.push(`height:${cssLen(spec.height)}`);
    d.push(`min-width:${spec.minWidth !== undefined ? cssLen(spec.minWidth) : '0px'}`);
    d.push(`min-height:${spec.minHeight !== undefined ? cssLen(spec.minHeight) : '0px'}`);
    if (spec.maxWidth !== undefined) d.push(`max-width:${cssLen(spec.maxWidth)}`);
    if (spec.maxHeight !== undefined) d.push(`max-height:${cssLen(spec.maxHeight)}`);

    for (const side of SIDES) {
        if (spec.margin?.[side] !== undefined) d.push(`margin-${side}:${cssLen(spec.margin[side])}`);
        if (spec.padding?.[side] !== undefined) d.push(`padding-${side}:${cssLen(spec.padding[side])}`);
        if (spec.border?.[side] !== undefined) {
            const w = spec.border[side];
            d.push(`border-${side}-width:${cssLen(w)}`);
            d.push(`border-${side}-style:${w ? 'solid' : 'none'}`);
        }
    }
    if (edgeHasAny(spec.border)) d.push('border-color:rgba(0,0,0,.35)');

    if (spec.position && spec.position !== 'static') d.push(`position:${spec.position}`);
    if (spec.offsets) {
        for (const side of SIDES) {
            if (spec.offsets[side] !== undefined) d.push(`${side}:${cssLen(spec.offsets[side])}`);
        }
    }

    if (spec.flexGrow !== undefined) d.push(`flex-grow:${spec.flexGrow}`);
    if (spec.flexShrink !== undefined) d.push(`flex-shrink:${spec.flexShrink}`);
    if (spec.flexBasis !== undefined) d.push(`flex-basis:${cssLen(spec.flexBasis)}`);
    if (spec.order !== undefined) d.push(`order:${spec.order}`);
    if (spec.alignSelf !== undefined) d.push(`align-self:${spec.alignSelf}`);

    // The engine has no baseline/strut model: LayoutLine puts every box on the line's top edge
    // and sizes the line from box heights alone (NormalFlowStrategy.cpp:14-21, 107-110). CSS
    // instead baseline-aligns inline-level boxes inside a line box that reserves room for the
    // strut's descender. `vertical-align:top` on the boxes and a zeroed font on their container
    // are what "no strut, top-aligned" looks like in CSS -- the same kind of initial-value
    // statement as min-width:0 above, not a fudge factor.
    if (INLINE_LEVEL.has(display)) d.push('vertical-align:top');
    if ((spec.children ?? []).some((c) => INLINE_LEVEL.has(c.display ?? 'block')))
        d.push('font-size:0', 'line-height:0');

    if (spec.color) d.push(`background:${spec.color}`);
    return d.join(';') + ';';
}

/// Inline-level siblings are emitted with no whitespace between them: collapsible whitespace
/// in the source becomes a rendered space in a line box, and the engine has no concept of it.
function renderHtmlNode(spec, parent, depth) {
    const children = spec.children ?? [];
    const inlineRun = children.some((c) => INLINE_LEVEL.has(c.display ?? 'block'));
    const pad = '  '.repeat(depth);
    const open = `${pad}<div id="${spec.id}" style="${styleDecls(spec, parent)}">`;
    if (children.length === 0) return `${open}</div>`;
    const inner = inlineRun
        ? children.map((c) => renderHtmlNode(c, spec, 0).trimStart()).join('')
        : '\n' + children.map((c) => renderHtmlNode(c, spec, depth + 1)).join('\n') + `\n${pad}`;
    return `${open}${inner}</div>`;
}

function renderHtmlPage(testCase) {
    // Byte-identical reset to the hand-written fixtures. box-sizing:border-box is not a style
    // choice: an explicit Px/Percent size in the engine *is* the border box (Node.cpp:384,
    // "Explicit Px/Percent sizes are border-box").
    return `<!doctype html><html><head><meta charset="utf-8"><title>${testCase.name}</title><style>
*{box-sizing:border-box;margin:0;padding:0;border:0 solid transparent}
html,body{margin:0;padding:0}
</style></head><body>
${renderHtmlNode(testCase.root, null, 0)}
</body></html>`;
}

// ---------------------------------------------------------------- C++ emission

const cppVar = (id) => 'v_' + id.replace(/[^A-Za-z0-9_]/g, '_');

function renderCppNode(spec, parentVar, out) {
    const v = cppVar(spec.id);
    const display = spec.display ?? 'block';
    out.push(`        auto ${v} = N("${spec.id}", OuterDisplay::${DISPLAY_CPP[display]});`);

    const dim = (field, value) => out.push(`        ${v}->GetStyle().Modify<Dimensions>().${field} = ${value};`);
    const flex = (field, value) => out.push(`        ${v}->GetStyle().Modify<CSSFlex>().${field} = ${value};`);

    if (FLEX_CONTAINERS.has(display)) {
        if (spec.direction) flex('Direction', `FlexDirection::${DIRECTION[spec.direction]}`);
        if (spec.wrap) flex('Wrap', `FlexWrap::${WRAP[spec.wrap]}`);
        if (spec.justify) flex('Justify', `JustifyContent::${JUSTIFY[spec.justify]}`);
        if (spec.alignItems) flex('Align', `AlignItems::${ALIGN_ITEMS[spec.alignItems]}`);
        if (spec.alignContent) flex('ContentAlign', `AlignContent::${ALIGN_CONTENT[spec.alignContent]}`);
        if (spec.gapRow !== undefined) flex('Gaps.Row', cppLen(spec.gapRow));
        if (spec.gapColumn !== undefined) flex('Gaps.Column', cppLen(spec.gapColumn));
    }

    if (spec.width !== undefined) dim('Width', cppLen(spec.width));
    if (spec.height !== undefined) dim('Height', cppLen(spec.height));
    if (spec.minWidth !== undefined) dim('MinWidth', cppLen(spec.minWidth));
    if (spec.minHeight !== undefined) dim('MinHeight', cppLen(spec.minHeight));
    if (spec.maxWidth !== undefined) dim('MaxWidth', cppLen(spec.maxWidth));
    if (spec.maxHeight !== undefined) dim('MaxHeight', cppLen(spec.maxHeight));

    const BORDER_FIELD = { top: 'WidthTop', right: 'WidthRight', bottom: 'WidthBottom', left: 'WidthLeft' };
    for (const side of SIDES) {
        const Side = side[0].toUpperCase() + side.slice(1);
        if (spec.margin?.[side] !== undefined)
            out.push(`        ${v}->GetStyle().Modify<MarginEdge>().${Side} = ${cppLen(spec.margin[side])};`);
        if (spec.padding?.[side] !== undefined)
            out.push(`        ${v}->GetStyle().Modify<PaddingEdge>().${Side} = ${cppLen(spec.padding[side])};`);
        if (spec.border?.[side] !== undefined)
            out.push(`        ${v}->GetStyle().Modify<BorderProperties>().${BORDER_FIELD[side]} = ${cppLen(spec.border[side])};`);
    }

    if (spec.position && spec.position !== 'static') dim('Position', `PositionType::${POSITION[spec.position]}`);
    // Only absolute insets are written: the Relative branch reads a struct with no public
    // setter, so emitting them would produce C++ that cannot express what the CSS says (R9).
    if (spec.position === 'absolute' && spec.offsets) {
        for (const side of SIDES) {
            if (spec.offsets[side] === undefined) continue;
            dim(side[0].toUpperCase() + side.slice(1), cppLen(spec.offsets[side]));
        }
    }

    if (spec.flexGrow !== undefined) flex('FlexGrow', fLit(spec.flexGrow));
    if (spec.flexShrink !== undefined) flex('FlexShrink', fLit(spec.flexShrink));
    if (spec.flexBasis !== undefined) flex('FlexBasis', cppLen(spec.flexBasis));
    if (spec.order !== undefined) flex('Order', String(spec.order));
    if (spec.alignSelf !== undefined) flex('AlignSelf', `AlignItems::${ALIGN_ITEMS[spec.alignSelf]}`);

    if (parentVar) out.push(`        ${parentVar}->AddChild(${v});`);
    out.push('');
    for (const child of spec.children ?? []) renderCppNode(child, v, out);
    return v;
}

function renderCppCase(testCase) {
    const out = [];
    const rootVar = renderCppNode(testCase.root, null, out);
    const w = testCase.calc?.w ?? testCase.root.width;
    const h = testCase.calc?.h ?? testCase.root.height;
    if (w === undefined || h === undefined || isPercent(w) || isPercent(h))
        throw new SpecError(`case "${testCase.name}": needs calc:{w,h} in px (the root's own size is auto or a percentage)`);
    return `    // ${testCase.title ?? testCase.name}
${testCase.note ? `    // ${testCase.note}\n` : ''}    void Fixture_${testCase.name}(float &W, float &H) {
        W = ${fLit(w)}; H = ${fLit(h)};
${out.join('\n')}
        ${rootVar}->Calculate(W, H);
    }
`;
}

// ---------------------------------------------------------------- drive

const specFiles = readdirSync(PAGES_DIR).filter((f) => f.endsWith('.mjs')).sort();
const cases = [];
for (const file of specFiles) {
    const mod = await import(pathToFileURL(path.join(PAGES_DIR, file)).href);
    const exported = mod.default;
    const list = Array.isArray(exported) ? exported : [exported];
    for (const c of list) cases.push(c);
}
if (cases.length === 0) throw new SpecError(`no cases found in ${PAGES_DIR}`);

const names = new Set();
for (const c of cases) {
    if (names.has(c.name)) throw new SpecError(`duplicate case name "${c.name}"`);
    names.add(c.name);
    validateCase(c);
}

mkdirSync(FIXTURES_DIR, { recursive: true });
for (const c of cases) {
    writeFileSync(path.join(FIXTURES_DIR, `${c.name}.html`), renderHtmlPage(c) + '\n', 'utf8');
}

const banner = (what) => `// GENERATED by tools/browser_layout_check/generate.mjs -- do not edit by hand.
// ${what} Re-run \`node generate.mjs\` (or build any target in this directory) after editing pages/.
`;

writeFileSync(path.join(__dirname, 'generated_cases.inc'),
    banner('Fixture bodies, #included inside harness.cpp\'s anonymous namespace.') +
    cases.map(renderCppCase).join('\n'), 'utf8');

writeFileSync(path.join(__dirname, 'generated_list.inc'),
    banner('Fixture registrations, spliced into harness.cpp\'s `fixtures` initialiser list.') +
    cases.map((c) => `        {"${c.name}", Fixture_${c.name}},`).join('\n') + '\n', 'utf8');

const nodeCount = (spec) => 1 + (spec.children ?? []).reduce((n, c) => n + nodeCount(c), 0);
for (const c of cases) console.log(`generated ${c.name}: ${nodeCount(c.root)} nodes`);
console.log(`Wrote ${cases.length} fixtures + generated_cases.inc + generated_list.inc`);

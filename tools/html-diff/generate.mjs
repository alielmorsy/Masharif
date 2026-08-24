// Generates paired HTML fixtures + a C++ harness from one shared list of test cases,
// so the HTML and the Masharif tree describe the *same* layout by construction.
//
// Run: node generate.mjs
// Outputs:
//   fixtures/<case>.html          - standalone browser-truth page
//   harness/generated_cases.cpp   - Masharif tree-builder + JSON-line emitter per case
//   harness/case_list.inc         - list of case function names for main.cpp

import { writeFileSync, mkdirSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import path from 'node:path';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const FIXTURES_DIR = path.join(__dirname, 'fixtures');
const HARNESS_DIR = path.join(__dirname, 'harness');
mkdirSync(FIXTURES_DIR, { recursive: true });
mkdirSync(HARNESS_DIR, { recursive: true });

// ---------- shared enum maps ----------
const JUSTIFY = {
  'flex-start': 'FlexStart', 'flex-end': 'FlexEnd', 'center': 'FlexCenter',
  'space-between': 'SpaceBetween', 'space-around': 'SpaceAround', 'space-evenly': 'SpaceEvenly',
};
const ALIGN_ITEMS = {
  'stretch': 'Stretch', 'flex-start': 'FlexStart', 'flex-end': 'FlexEnd',
  'center': 'FlexCenter', 'baseline': 'Baseline', 'auto': 'AutoAlign',
};
const ALIGN_CONTENT = {
  'stretch': 'Stretch', 'flex-start': 'FlexStart', 'flex-end': 'FlexEnd',
  'center': 'FlexCenter', 'space-between': 'SpaceBetween', 'space-around': 'SpaceAround',
  'space-evenly': 'SpaceEvenly',
};
const DIRECTION = { row: 'Row', 'row-reverse': 'RowReverse', column: 'Column', 'column-reverse': 'ColumnReverse' };
const WRAP = { nowrap: 'NoWrap', wrap: 'Wrap', 'wrap-reverse': 'WrapReverse' };
const POSITION = { static: 'Static', relative: 'Relative', absolute: 'Absolute' };

function numLit(v) {
  const n = Number(v);
  return (Number.isInteger(n) ? n.toFixed(1) : String(n)) + 'f';
}
function cssValueCpp(v) {
  if (v === undefined || v === null || v === 'auto') return 'CSSValue()';
  if (typeof v === 'string' && v.endsWith('%')) return `CSSValue(${numLit(parseFloat(v))}, CSSUnit::Percent)`;
  return `CSSValue(${numLit(v)})`;
}
function cssValueCss(v) {
  if (v === undefined || v === null || v === 'auto') return 'auto';
  if (typeof v === 'string' && v.endsWith('%')) return v;
  return `${Number(v)}px`;
}

let varCounter = 0;
function nextVar() { return `n${varCounter++}`; }

// ---------- HTML rendering ----------
const CSS_DISPLAY = { flex: 'flex', block: 'block', none: 'none' };
function renderHtmlNode(spec, isRoot) {
  const s = [];
  s.push(`display:${CSS_DISPLAY[spec.display] ?? 'block'};`);
  if (spec.display === 'flex') {
    s.push(`flex-direction:${spec.direction ?? 'row'};`);
    s.push(`flex-wrap:${spec.wrap ?? 'nowrap'};`);
    s.push(`justify-content:${spec.justify ?? 'flex-start'};`);
    s.push(`align-items:${spec.alignItems ?? 'stretch'};`);
    s.push(`align-content:${spec.alignContent ?? 'stretch'};`);
    if (spec.gapRow !== undefined) s.push(`row-gap:${spec.gapRow}px;`);
    if (spec.gapColumn !== undefined) s.push(`column-gap:${spec.gapColumn}px;`);
  }
  s.push(`box-sizing:content-box;`);
  s.push(`width:${cssValueCss(spec.width)};`);
  s.push(`height:${cssValueCss(spec.height)};`);
  s.push(`min-width:${spec.minWidth !== undefined ? cssValueCss(spec.minWidth) : '0px'};`);
  s.push(`min-height:${spec.minHeight !== undefined ? cssValueCss(spec.minHeight) : '0px'};`);
  if (spec.maxWidth !== undefined) s.push(`max-width:${cssValueCss(spec.maxWidth)};`);
  if (spec.maxHeight !== undefined) s.push(`max-height:${cssValueCss(spec.maxHeight)};`);

  const m = spec.margin ?? {};
  s.push(`margin-top:${cssValueCss(m.top ?? 0)};`);
  s.push(`margin-right:${cssValueCss(m.right ?? 0)};`);
  s.push(`margin-bottom:${cssValueCss(m.bottom ?? 0)};`);
  s.push(`margin-left:${cssValueCss(m.left ?? 0)};`);

  const p = spec.padding ?? {};
  s.push(`padding-top:${cssValueCss(p.top ?? 0)};`);
  s.push(`padding-right:${cssValueCss(p.right ?? 0)};`);
  s.push(`padding-bottom:${cssValueCss(p.bottom ?? 0)};`);
  s.push(`padding-left:${cssValueCss(p.left ?? 0)};`);

  const b = spec.border ?? {};
  for (const [side, key] of [['top', 't'], ['right', 'r'], ['bottom', 'b'], ['left', 'l']]) {
    const w = b[side] ?? 0;
    s.push(`border-${side}-width:${w}px;`);
    s.push(`border-${side}-style:${w ? 'solid' : 'none'};`);
  }
  if (Object.values(b).some(Boolean)) s.push(`border-color:rgba(0,0,0,0.35);`);

  s.push(`position:${spec.position ?? 'static'};`);
  if (spec.position === 'absolute' || spec.position === 'relative') {
    const o = spec.offsets ?? {};
    if (o.top !== undefined) s.push(`top:${cssValueCss(o.top)};`);
    if (o.right !== undefined) s.push(`right:${cssValueCss(o.right)};`);
    if (o.bottom !== undefined) s.push(`bottom:${cssValueCss(o.bottom)};`);
    if (o.left !== undefined) s.push(`left:${cssValueCss(o.left)};`);
  }

  if (spec.display === 'flex') {
    // these only apply as flex-item properties, but harmless when unused (parent isn't flex)
  }
  if (spec.flexGrow !== undefined) s.push(`flex-grow:${spec.flexGrow};`);
  if (spec.flexShrink !== undefined) s.push(`flex-shrink:${spec.flexShrink};`);
  if (spec.flexBasis !== undefined) s.push(`flex-basis:${cssValueCss(spec.flexBasis)};`);
  if (spec.order !== undefined) s.push(`order:${spec.order};`);
  if (spec.alignSelf !== undefined) s.push(`align-self:${spec.alignSelf};`);

  if (spec.color) s.push(`background:${spec.color};`);
  s.push(`overflow:hidden;`);
  if (spec.label && spec.display !== 'none') {
    s.push(`color:#111;font:12px/1.3 system-ui,sans-serif;`);
    if (spec.display === 'flex') { s.push(`align-items:${spec.alignItems ?? 'stretch'};justify-content:${spec.justify ?? 'flex-start'};`); }
  }

  const style = s.join('');
  const childrenHtml = (spec.children ?? []).map((c) => renderHtmlNode(c, false)).join('\n');
  const label = spec.label ? escapeHtml(spec.label) : '';
  return `<div data-id="${spec.id}" style="${style}">${label}${childrenHtml}</div>`;
}
function escapeHtml(str) { return String(str).replace(/&/g, '&amp;').replace(/</g, '&lt;'); }

function renderHtmlPage(testCase) {
  const body = renderHtmlNode(testCase.root, true);
  return `<!doctype html>
<html><head><meta charset="utf-8"><title>${testCase.name}</title>
<style>
  * { box-sizing: content-box; margin: 0; padding: 0; border: 0 none; }
  html,body { background:#f4f4f6; }
  body { padding: 24px; font-family: system-ui, sans-serif; }
  h1 { font: 600 14px/1.4 system-ui, sans-serif; color:#333; margin-bottom:12px !important; }
  .stage-label { font: 12px/1.4 monospace; color:#888; margin-bottom:8px !important; }
</style>
</head><body>
<h1>${testCase.title ?? testCase.name}</h1>
<div class="stage-label">${testCase.note ?? ''}</div>
${body}
</body></html>`;
}

// ---------- C++ rendering ----------
function renderCppNode(spec, parentVar, lines, emits, caseName) {
  const v = nextVar();
  lines.push(`    auto ${v} = std::make_shared<Node>();`);
  const CPP_DISPLAY = { flex: 'Flex', block: 'Block', none: 'None' };
  lines.push(`    ${v}->SetDisplay(OuterDisplay::${CPP_DISPLAY[spec.display] ?? 'Block'});`);
  if (spec.display === 'flex') {
    if (spec.direction) lines.push(`    ${v}->GetStyle().Modify<CSSFlex>().Direction = FlexDirection::${DIRECTION[spec.direction]};`);
    if (spec.wrap) lines.push(`    ${v}->GetStyle().Modify<CSSFlex>().Wrap = FlexWrap::${WRAP[spec.wrap]};`);
    if (spec.justify) lines.push(`    ${v}->GetStyle().Modify<CSSFlex>().Justify = JustifyContent::${JUSTIFY[spec.justify]};`);
    if (spec.alignItems) lines.push(`    ${v}->GetStyle().Modify<CSSFlex>().Align = AlignItems::${ALIGN_ITEMS[spec.alignItems]};`);
    if (spec.alignContent) lines.push(`    ${v}->GetStyle().Modify<CSSFlex>().ContentAlign = AlignContent::${ALIGN_CONTENT[spec.alignContent]};`);
    if (spec.gapRow !== undefined) lines.push(`    ${v}->GetStyle().Modify<CSSFlex>().Gaps.Row = CSSValue(${numLit(spec.gapRow)});`);
    if (spec.gapColumn !== undefined) lines.push(`    ${v}->GetStyle().Modify<CSSFlex>().Gaps.Column = CSSValue(${numLit(spec.gapColumn)});`);
  }
  if (spec.flexGrow !== undefined) lines.push(`    ${v}->GetStyle().Modify<CSSFlex>().FlexGrow = ${numLit(spec.flexGrow)};`);
  if (spec.flexShrink !== undefined) lines.push(`    ${v}->GetStyle().Modify<CSSFlex>().FlexShrink = ${numLit(spec.flexShrink)};`);
  if (spec.flexBasis !== undefined) lines.push(`    ${v}->GetStyle().Modify<CSSFlex>().FlexBasis = ${cssValueCpp(spec.flexBasis)};`);
  if (spec.order !== undefined) lines.push(`    ${v}->GetStyle().Modify<CSSFlex>().Order = ${spec.order};`);
  if (spec.alignSelf !== undefined) lines.push(`    ${v}->GetStyle().Modify<CSSFlex>().AlignSelf = AlignItems::${ALIGN_ITEMS[spec.alignSelf]};`);

  if (spec.width !== undefined) lines.push(`    ${v}->GetStyle().Modify<Dimensions>().Width = ${cssValueCpp(spec.width)};`);
  if (spec.height !== undefined) lines.push(`    ${v}->GetStyle().Modify<Dimensions>().Height = ${cssValueCpp(spec.height)};`);
  if (spec.minWidth !== undefined) lines.push(`    ${v}->GetStyle().Modify<Dimensions>().MinWidth = ${cssValueCpp(spec.minWidth)};`);
  if (spec.minHeight !== undefined) lines.push(`    ${v}->GetStyle().Modify<Dimensions>().MinHeight = ${cssValueCpp(spec.minHeight)};`);
  if (spec.maxWidth !== undefined) lines.push(`    ${v}->GetStyle().Modify<Dimensions>().MaxWidth = ${cssValueCpp(spec.maxWidth)};`);
  if (spec.maxHeight !== undefined) lines.push(`    ${v}->GetStyle().Modify<Dimensions>().MaxHeight = ${cssValueCpp(spec.maxHeight)};`);

  if (spec.margin) {
    const m = spec.margin;
    for (const side of ['top', 'right', 'bottom', 'left']) {
      if (m[side] === undefined) continue;
      const Side = side[0].toUpperCase() + side.slice(1);
      lines.push(`    ${v}->GetStyle().Modify<MarginEdge>().${Side} = ${cssValueCpp(m[side])};`);
    }
  }
  if (spec.padding) {
    const p = spec.padding;
    for (const side of ['top', 'right', 'bottom', 'left']) {
      if (p[side] === undefined) continue;
      const Side = side[0].toUpperCase() + side.slice(1);
      lines.push(`    ${v}->GetStyle().Modify<PaddingEdge>().${Side} = ${cssValueCpp(p[side])};`);
    }
  }
  if (spec.border) {
    const b = spec.border;
    const map = { top: 'WidthTop', right: 'WidthRight', bottom: 'WidthBottom', left: 'WidthLeft' };
    for (const side of ['top', 'right', 'bottom', 'left']) {
      if (b[side] === undefined) continue;
      lines.push(`    ${v}->GetStyle().Modify<BorderProperties>().${map[side]} = ${cssValueCpp(b[side])};`);
    }
  }

  if (spec.position) lines.push(`    ${v}->GetStyle().Modify<Dimensions>().Position = PositionType::${POSITION[spec.position]};`);
  if (spec.position === 'absolute' && spec.offsets) {
    const o = spec.offsets;
    for (const side of ['top', 'right', 'bottom', 'left']) {
      if (o[side] === undefined) continue;
      const Side = side[0].toUpperCase() + side.slice(1);
      lines.push(`    ${v}->GetStyle().Modify<Dimensions>().${Side} = ${cssValueCpp(o[side])};`);
    }
  }
  // NOTE: position:relative + offsets is intentionally NOT wired here -- see the
  // dedicated relative_offset_dead_field case, which documents that Style has no
  // public setter reaching m_Offsets (PositionOffsets), so a relative top/left is a no-op.

  if (parentVar) lines.push(`    ${parentVar}->AddChild(${v});`);
  emits.push(`    Emit("${caseName}", "${spec.id}", *${v});`);

  for (const child of spec.children ?? []) renderCppNode(child, v, lines, emits, caseName);
  return v;
}

function renderCppCase(testCase) {
  varCounter = 0;
  const lines = [];
  const emits = [];
  const rootVar = renderCppNode(testCase.root, null, lines, emits, testCase.name);
  const calcW = testCase.calc?.w ?? testCase.root.width ?? 400;
  const calcH = testCase.calc?.h ?? testCase.root.height ?? 300;
  const fn = `Case_${testCase.name}`;
  return `void ${fn}() {
${lines.join('\n')}
    ${rootVar}->Calculate(${numLit(calcW)}, ${numLit(calcH)});
${emits.join('\n')}
}
`;
}

// ---------- test case list ----------
const cases = [];

cases.push({
  name: 'row_space_between',
  title: 'Flex row · justify-content:space-between',
  root: {
    id: 'root', display: 'flex', direction: 'row', justify: 'space-between', alignItems: 'center',
    width: 400, height: 120, color: '#e8ecf7',
    children: [
      { id: 'a', display: 'flex', width: 80, height: 60, color: '#5b7fd8', label: 'A' },
      { id: 'b', display: 'flex', width: 80, height: 80, color: '#7f5bd8', label: 'B' },
      { id: 'c', display: 'flex', width: 80, height: 40, color: '#d85b9a', label: 'C' },
    ],
  },
});

cases.push({
  name: 'column_gap_center',
  title: 'Flex column · align-items:center + row-gap',
  root: {
    id: 'root', display: 'flex', direction: 'column', alignItems: 'center', gapRow: 16,
    width: 300, height: 320, padding: { top: 12, bottom: 12, left: 12, right: 12 }, color: '#eef7ec',
    children: [
      { id: 'a', display: 'flex', width: 220, height: 60, color: '#4caf50', label: 'header' },
      { id: 'b', display: 'flex', width: 140, height: 60, color: '#66bb6a', label: 'mid' },
      { id: 'c', display: 'flex', width: 180, height: 60, color: '#81c784', label: 'footer' },
    ],
  },
});

cases.push({
  name: 'wrap_card_grid',
  title: 'Flex-wrap card grid · align-content:space-around',
  root: {
    id: 'root', display: 'flex', direction: 'row', wrap: 'wrap', alignContent: 'space-around',
    justify: 'flex-start', gapRow: 10, gapColumn: 10,
    width: 340, height: 260, padding: { top: 10, bottom: 10, left: 10, right: 10 }, color: '#fff7e6',
    children: Array.from({ length: 6 }, (_, i) => ({
      id: `card${i}`, display: 'flex', width: 100, height: 70,
      color: ['#ffb74d', '#ffa726', '#fb8c00', '#f57c00', '#ef6c00', '#e65100'][i], label: `${i}`,
    })),
  },
});

cases.push({
  name: 'holy_grail',
  title: 'Holy-grail layout: header/sidebar/main/footer',
  root: {
    id: 'root', display: 'flex', direction: 'column', width: 480, height: 360, color: '#f0f0f0',
    children: [
      { id: 'header', display: 'flex', height: 50, flexGrow: 0, flexShrink: 0, color: '#37474f', label: 'header' },
      {
        id: 'middle', display: 'flex', direction: 'row', flexGrow: 1, flexShrink: 1, minHeight: 0, color: '#eceff1',
        children: [
          { id: 'sidebar', display: 'flex', width: 120, flexShrink: 0, color: '#607d8b', label: 'nav' },
          { id: 'main', display: 'flex', flexGrow: 1, flexShrink: 1, minWidth: 0, color: '#cfd8dc', label: 'content' },
        ],
      },
      { id: 'footer', display: 'flex', height: 40, flexGrow: 0, flexShrink: 0, color: '#37474f', label: 'footer' },
    ],
  },
});

cases.push({
  name: 'align_self_overrides',
  title: 'align-self overriding container align-items',
  root: {
    id: 'root', display: 'flex', direction: 'row', alignItems: 'flex-start',
    width: 360, height: 160, color: '#f3e5f5',
    children: [
      { id: 'a', display: 'flex', width: 70, height: 60, color: '#ab47bc', label: 'start' },
      { id: 'b', display: 'flex', width: 70, height: 60, alignSelf: 'center', color: '#8e24aa', label: 'center' },
      { id: 'c', display: 'flex', width: 70, height: 60, alignSelf: 'flex-end', color: '#6a1b9a', label: 'end' },
      { id: 'd', display: 'flex', width: 70, alignSelf: 'stretch', color: '#4a148c', label: 'stretch' },
    ],
  },
});

cases.push({
  name: 'margin_auto_centering',
  title: 'margin:auto centering inside a flex row',
  root: {
    id: 'root', display: 'flex', direction: 'row', width: 300, height: 150, color: '#e0f7fa',
    children: [
      { id: 'box', display: 'flex', width: 100, height: 60, margin: { left: 'auto', right: 'auto', top: 'auto', bottom: 'auto' }, color: '#00838f', label: 'centered' },
    ],
  },
});

cases.push({
  name: 'absolute_offsets',
  title: 'position:absolute with mixed top/right/bottom/left offsets',
  root: {
    id: 'root', display: 'flex', position: 'relative', width: 320, height: 220, color: '#fce4ec',
    children: [
      { id: 'tl', display: 'flex', position: 'absolute', offsets: { top: 10, left: 10 }, width: 60, height: 40, color: '#d81b60', label: 'TL' },
      { id: 'br', display: 'flex', position: 'absolute', offsets: { bottom: 10, right: 10 }, width: 60, height: 40, color: '#ad1457', label: 'BR' },
      { id: 'centerish', display: 'flex', position: 'absolute', offsets: { top: '25%', left: '25%', right: '25%', bottom: '25%' }, color: '#f06292', label: 'stretch-inset' },
    ],
  },
});

// Deliberate known-gap demo: position:relative + top/left offset.
// Style::Modify<T>() only allows {Dimensions, CSSFlex, MarginEdge, PaddingEdge, Edge,
// BorderProperties} (structure/Style.h ~line 36-42) -- PositionOffsets is not in that
// list, so m_Offsets (read by Node::LayoutImpl for the Relative branch, Node.cpp:307-312)
// can never be written through the public API. A relative top/left is silently a no-op.
cases.push({
  name: 'relative_offset_dead_field',
  title: 'KNOWN GAP: position:relative + top/left offset',
  note: 'Browser shifts the pink box by (top:20,left:15). Masharif cannot: PositionOffsets has no public setter.',
  root: {
    id: 'root', display: 'flex', width: 260, height: 140, color: '#ede7f6',
    children: [
      { id: 'anchor', display: 'flex', width: 80, height: 80, color: '#c5cae9', label: 'static sibling' },
      { id: 'shifted', display: 'flex', position: 'relative', offsets: { top: 20, left: 15 }, width: 80, height: 80, color: '#ff8a80', label: 'relative' },
    ],
  },
});

cases.push({
  name: 'percent_sizing',
  title: 'Percentage width/height children',
  root: {
    id: 'root', display: 'flex', direction: 'row', width: 400, height: 200, color: '#e8f5e9',
    children: [
      { id: 'a', display: 'flex', width: '25%', height: '50%', color: '#43a047', label: '25%/50%' },
      { id: 'b', display: 'flex', width: '50%', height: '100%', color: '#2e7d32', label: '50%/100%' },
      { id: 'c', display: 'flex', width: '25%', height: '75%', color: '#1b5e20', label: '25%/75%' },
    ],
  },
});

cases.push({
  name: 'min_max_shrink_grow',
  title: 'min/max clamping under flex-grow and flex-shrink',
  root: {
    id: 'root', display: 'flex', direction: 'row', width: 400, height: 100, color: '#fff3e0',
    children: [
      { id: 'a', display: 'flex', width: 200, flexShrink: 1, minWidth: 120, height: 60, color: '#fb8c00', label: 'minW 120' },
      { id: 'b', display: 'flex', width: 200, flexShrink: 1, minWidth: 0, height: 60, color: '#f57c00', label: 'minW 0' },
      { id: 'c', display: 'flex', width: 40, flexGrow: 1, maxWidth: 90, height: 60, color: '#ef6c00', label: 'maxW 90' },
    ],
  },
});

cases.push({
  name: 'box_model_border_padding',
  title: 'Content-box model: border + padding nesting',
  root: {
    id: 'root', display: 'flex', direction: 'row', width: 300, height: 160,
    padding: { top: 15, bottom: 15, left: 15, right: 15 }, border: { top: 6, bottom: 6, left: 6, right: 6 }, color: '#e1f5fe',
    children: [
      { id: 'inner', display: 'flex', width: 100, height: 80, padding: { top: 8, bottom: 8, left: 8, right: 8 }, border: { top: 3, bottom: 3, left: 3, right: 3 }, color: '#0288d1', label: 'inner' },
    ],
  },
});

cases.push({
  name: 'direction_reverse',
  title: 'flex-direction: row-reverse vs column-reverse',
  root: {
    id: 'root', display: 'flex', direction: 'row', width: 420, height: 220, color: '#fbe9e7',
    children: [
      {
        id: 'row', display: 'flex', direction: 'row-reverse', width: 200, height: 200, gapColumn: 6, color: '#ffccbc',
        children: [
          { id: 'r1', display: 'flex', width: 50, height: 50, color: '#ff7043', label: '1' },
          { id: 'r2', display: 'flex', width: 50, height: 50, color: '#f4511e', label: '2' },
          { id: 'r3', display: 'flex', width: 50, height: 50, color: '#d84315', label: '3' },
        ],
      },
      {
        id: 'col', display: 'flex', direction: 'column-reverse', width: 200, height: 200, gapRow: 6, color: '#d7ccc8',
        children: [
          { id: 'c1', display: 'flex', width: 50, height: 40, color: '#8d6e63', label: '1' },
          { id: 'c2', display: 'flex', width: 50, height: 40, color: '#6d4c41', label: '2' },
          { id: 'c3', display: 'flex', width: 50, height: 40, color: '#4e342e', label: '3' },
        ],
      },
    ],
  },
});

cases.push({
  name: 'order_reorder',
  title: 'order property reorders visual position, not DOM position',
  root: {
    id: 'root', display: 'flex', direction: 'row', width: 300, height: 100, color: '#f1f8e9',
    children: [
      { id: 'first_dom', display: 'flex', order: 3, width: 80, height: 60, color: '#9ccc65', label: 'DOM#1 order3' },
      { id: 'second_dom', display: 'flex', order: 1, width: 80, height: 60, color: '#7cb342', label: 'DOM#2 order1' },
      { id: 'third_dom', display: 'flex', order: 2, width: 80, height: 60, color: '#558b2f', label: 'DOM#3 order2' },
    ],
  },
});

cases.push({
  name: 'block_normal_flow',
  title: 'display:block normal flow: vertical stack + margin-bottom spacing',
  note: 'c2 has margin-left:20 -- known gap: NormalFlowStrategy never applies horizontal margin to block children (see report).',
  root: {
    id: 'root', display: 'block', width: 240, color: '#eceff1',
    children: [
      { id: 'c0', display: 'block', height: 40, margin: { bottom: 12 }, color: '#78909c', label: 'c0' },
      { id: 'c1', display: 'block', height: 40, margin: { bottom: 12 }, color: '#546e7a', label: 'c1' },
      { id: 'c2', display: 'block', height: 40, width: 100, margin: { left: 20 }, color: '#37474f', label: 'c2 ml20' },
    ],
  },
  calc: { w: 240, h: 300 },
});

// Exact tree from tests/OutOfFlowRepositionTests.cpp: absolute_both_insets_zero_centers_flex_child
cases.push({
  name: 'absolute_zero_insets_bar',
  title: 'Regression: absolute flex bar with Left=0 & Right=0 stretches, then centers its child',
  root: {
    id: 'root', display: 'flex', position: 'relative', width: 200, height: 200, color: '#eceff1',
    children: [
      {
        id: 'bar', display: 'flex', position: 'absolute', offsets: { left: 0, right: 0, top: 12 },
        direction: 'row', justify: 'center', alignItems: 'center', color: '#455a64',
        children: [
          { id: 'pill', display: 'flex', width: 60, height: 28, color: '#ffca28', label: 'pill' },
        ],
      },
    ],
  },
});

// Exact tree from tests/OutOfFlowRepositionTests.cpp: display_none_absolute_child_is_not_positioned,
// plus an in-flow display:none sibling to confirm it takes no flow space either.
cases.push({
  name: 'display_none_child',
  title: 'display:none removes both in-flow and absolute boxes from calculation',
  root: {
    id: 'root', display: 'flex', direction: 'row', position: 'relative', width: 300, height: 100, color: '#f9fbe7',
    children: [
      { id: 'a', display: 'flex', width: 60, height: 60, color: '#c0ca33', label: 'A' },
      { id: 'hidden', display: 'none', width: 60, height: 60, color: '#e53935', label: 'hidden' },
      { id: 'c', display: 'flex', width: 60, height: 60, color: '#9e9d24', label: 'C' },
      { id: 'absHidden', display: 'none', position: 'absolute', offsets: { top: 20, left: 20 }, width: 20, height: 20, color: '#e53935' },
    ],
  },
});

// ---------- emit ----------
for (const c of cases) {
  writeFileSync(path.join(FIXTURES_DIR, `${c.name}.html`), renderHtmlPage(c), 'utf8');
}

const cppCases = cases.map(renderCppCase).join('\n');
const cppHeader = `// GENERATED by tools/html-diff/generate.mjs -- do not edit by hand.
#include <masharifcore/Masharif.h>
#include <cstdio>
#include <memory>
using namespace masharif;

static void Emit(const char* caseName, const char* id, Node& n) {
    auto& l = n.GetLayout();
    std::printf("{\\"case\\":\\"%s\\",\\"id\\":\\"%s\\",\\"x\\":%.4f,\\"y\\":%.4f,\\"w\\":%.4f,\\"h\\":%.4f}\\n",
        caseName, id, l.ComputedX, l.ComputedY, l.ComputedWidth, l.ComputedHeight);
}

${cppCases}
void RunAllCases() {
${cases.map((c) => `    Case_${c.name}();`).join('\n')}
}
`;
writeFileSync(path.join(HARNESS_DIR, 'generated_cases.cpp'), cppHeader, 'utf8');
writeFileSync(path.join(HARNESS_DIR, 'case_names.json'), JSON.stringify(cases.map(c => ({name: c.name, title: c.title, note: c.note ?? null})), null, 2), 'utf8');

console.log(`Generated ${cases.length} fixtures + harness/generated_cases.cpp`);

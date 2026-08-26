// Small fixtures that assert divergences on purpose.
//
// The full-page specs in this directory obey authoring rules (R5-R12, see README.md) that steer
// around places where the engine and CSS answer different questions. Steering around them
// silently would be the wrong kind of green: the rules would read as arbitrary style, and a fix
// or a regression in any of these paths would go unnoticed. So each rule that shapes a page gets
// one minimal case here that walks straight into it, opts in with `allowGap`, and is declared in
// compare.mjs's KNOWN_GAPS with the engine line responsible.
//
// If one of these starts matching, compare.mjs says "GAP CLOSED" -- that is the signal to delete
// the case, the KNOWN_GAPS entry and the rule together.

const INK = '#37474f';
const TINT = '#cfd8dc';

export default [
    // R7 -- two separate divergences in one three-box stack:
    //   b_second: the block branch writes LocalY without the child's own margin-top
    //             (NormalFlowStrategy.cpp:95-99), so 20px of margin-top does not move it.
    //   b_third:  CSS collapses b_first's 12px margin-bottom against that 20px margin-top into a
    //             single 20px gap; the engine has no collapsing and adds both, so everything
    //             below ends up 12px further down.
    {
        name: 'gap_block_margin_top',
        title: 'KNOWN GAP: block margin-top is applied to the next sibling, and never collapses',
        note: 'Browser: 0 / 60 / 100. Engine: 0 / 52 / 112.',
        calc: { w: 200, h: 200 },
        root: {
            id: 'root', display: 'block', width: 200, height: 200,
            children: [
                { id: 'b_first', display: 'block', height: 40, margin: { bottom: 12 }, color: INK },
                {
                    id: 'b_second', display: 'block', height: 40, margin: { top: 20 },
                    allowGap: ['R7'], color: TINT,
                },
                { id: 'b_third', display: 'block', height: 40, color: INK },
            ],
        },
    },

    // R5 -- LayoutLine starts its cursor at the container's border-box origin rather than its
    // content-box origin (NormalFlowStrategy.cpp:12-13), so a padded container does not indent
    // the inline-level boxes on its lines. Both chips land 16px up and to the left of the browser.
    {
        name: 'gap_inline_container_padding',
        title: 'KNOWN GAP: an inline-level line ignores its container\'s padding',
        note: 'Browser: chips at (16,16) and (76,16). Engine: (0,0) and (60,0).',
        calc: { w: 240, h: 120 },
        root: {
            id: 'root', display: 'block', width: 240, height: 120,
            padding: { top: 16, right: 16, bottom: 16, left: 16 },
            allowGap: ['R5'], color: '#eceff1',
            children: [
                { id: 'chip_a', display: 'inline-block', width: 60, height: 30, color: INK },
                { id: 'chip_b', display: 'inline-block', width: 60, height: 30, color: TINT },
            ],
        },
    },

    // R8 -- ApplyBlockAutoHeight measures from the child's LocalY, which the block branch already
    // offset by padding-top, and then adds padding-top again (Node.cpp:447-451 vs
    // NormalFlowStrategy.cpp:96). The child's position is right; the derived height is 20px long.
    {
        name: 'gap_block_auto_height_padding',
        title: 'KNOWN GAP: an AUTO-height block counts its padding-top twice',
        note: 'Browser: wrapper is 80px tall (20 + 40 + 20). Engine: 100px. The child sits at y=20 in both.',
        calc: { w: 200, h: 200 },
        root: {
            id: 'root', display: 'block', width: 200, height: 200,
            children: [
                {
                    id: 'wrapper', display: 'block',
                    padding: { top: 20, bottom: 20 }, allowGap: ['R8'], color: '#eceff1',
                    children: [{ id: 'inner', display: 'block', height: 40, color: INK }],
                },
            ],
        },
    },
];

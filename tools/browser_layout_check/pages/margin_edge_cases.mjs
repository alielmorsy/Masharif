// Full-page case: the margin paths the gallery page never reaches -- auto main-axis margins next
// to a flexible sibling, next to justify-content, and on an overflowing line; and non-auto margins
// on out-of-flow boxes, which are terms in the same inset constraint equation as top/right/bottom/
// left. Every expectation here is a CSS rule the engine had no fixture for, so the browser is the
// only thing that says whether the reading of the spec is right.
//
// Band 1 -- flex auto margins:
//   am_grow      flexible lengths resolve BEFORE auto margins claim space (9.7 then 8.1), so the
//                grow item takes the free space and the auto margin gets nothing
//   am_justify   auto margins absorb all the positive free space, leaving justify-content nothing
//   am_gm        gaps and non-auto margins are consumed by the line, so neither is free space
//   am_of        an overflowing line has no positive free space: auto margins resolve to 0
// Band 2 -- out-of-flow margins (CSS 2.1 10.3.7 / 10.6.4):
//   abm_inset    the margin on the anchoring side insets the border box from that inset
//   abm_static   with no insets, alignment distributes the MARGIN box
//   abm_single   over-constrained with one auto margin: it takes the whole leftover

const C = {
    page: '#fbfbfd', tile: '#ffffff', line: '#dcdfe6',
    a: '#3d5afe', b: '#1fae7a', c: '#ff8a3d', d: '#e0245e',
};

/// A demo tile. No padding or border anywhere in this page: band 2's tiles are absolute containing
/// blocks, which R12 requires to be unpadded, and band 1 gains nothing from being padded.
const tile = (id, w, h, extra = {}) => ({
    id, display: 'flex', direction: 'row', width: w, height: h,
    flexGrow: 0, flexShrink: 0, color: C.tile, ...extra,
});

const band = (id, h, children) => ({
    id, display: 'flex', direction: 'row', gapColumn: 20,
    height: h, flexGrow: 0, flexShrink: 0, children,
});

export default {
    name: 'page_margin_edge_cases',
    title: 'Full page: auto margins against flex/justify/overflow, and margins on out-of-flow boxes',
    note: 'Two bands: main-axis auto margins in four configurations the gallery page does not cover, and non-auto margins on absolutely positioned boxes anchored by a start inset, an end inset, and no inset at all.',
    calc: { w: 1100, h: 320 },
    root: {
        id: 'root', display: 'flex', direction: 'column', gapRow: 20, alignItems: 'flex-start',
        width: 1100, height: 320, color: C.page,
        children: [
            band('band_flex', 60, [
                // 300 - 30 - 70 = 200 of free space. The grow item resolves first and takes all of
                // it, so margin-left:auto has nothing left and am_grow_b does not move.
                tile('am_grow', 300, 60, {
                    children: [
                        { id: 'am_grow_a', display: 'flex', width: 30, height: 40, flexGrow: 1, color: C.a },
                        {
                            id: 'am_grow_b', display: 'flex', width: 70, height: 40, color: C.b,
                            margin: { left: 'auto' },
                        },
                    ],
                }),
                // justify-content:center would offset by 100; the auto margin already spent all 200,
                // so the correct answer is 200, not 300.
                tile('am_justify', 300, 60, { justify: 'center', children: [
                    {
                        id: 'am_justify_a', display: 'flex', width: 100, height: 40, color: C.c,
                        margin: { left: 'auto' },
                    },
                ] }),
                // 300 - 70 - 70 - 20 (gap) - 10 (am_gm_b's own margin-left) = 130 for the one auto
                // margin -- neither the gap nor a declared margin is free space.
                tile('am_gm', 300, 60, { gapColumn: 20, children: [
                    {
                        id: 'am_gm_a', display: 'flex', width: 70, height: 40, color: C.a,
                        margin: { right: 'auto' },
                    },
                    {
                        id: 'am_gm_b', display: 'flex', width: 70, height: 40, color: C.d,
                        margin: { left: 10 },
                    },
                ] }),
                // 140 of content in a 100 box: free space is negative, so the auto margin is 0 and
                // the items pack from the start edge and overflow.
                tile('am_of', 100, 60, { children: [
                    {
                        id: 'am_of_a', display: 'flex', width: 70, height: 40, flexShrink: 0, color: C.b,
                        margin: { right: 'auto' },
                    },
                    { id: 'am_of_b', display: 'flex', width: 70, height: 40, flexShrink: 0, color: C.c },
                ] }),
            ]),
            band('band_abs', 200, [
                // abm_start: 10 + 5 = 15 and 20 + 7 = 27. abm_end is pulled BACK from its end
                // insets by its trailing margins: 300 - 10 - 5 - 100 = 185, 200 - 20 - 7 - 50 = 123.
                tile('abm_inset', 300, 200, { position: 'relative', color: C.line, children: [
                    {
                        id: 'abm_start', display: 'flex', position: 'absolute',
                        offsets: { top: 20, left: 10 }, width: 100, height: 50, color: C.a,
                        margin: { left: 5, top: 7 },
                    },
                    {
                        id: 'abm_end', display: 'flex', position: 'absolute',
                        offsets: { right: 10, bottom: 20 }, width: 100, height: 50, color: C.b,
                        margin: { right: 5, bottom: 7 },
                    },
                ] }),
                // No insets: the 140x70 margin box is centred in 300x200 (free 160x130, half 80x65)
                // and the leading margin then insets the border box -- 80 + 10 = 90, 65 + 8 = 73.
                tile('abm_static', 300, 200, {
                    position: 'relative', justify: 'center', alignItems: 'center', color: C.line,
                    children: [{
                        id: 'abm_static_child', display: 'flex', position: 'absolute',
                        width: 100, height: 50, color: C.c,
                        margin: { left: 10, right: 30, top: 8, bottom: 12 },
                    }],
                }),
                // Over-constrained on both axes. Horizontally one auto margin takes the whole
                // leftover: 300 - 10 - 20 - 100 - 30 = 140, so x = 10 + 140 = 150. Vertically both
                // margins are auto and split it: (200 - 50) / 2 = 75.
                tile('abm_single', 300, 200, { position: 'relative', color: C.line, children: [{
                    id: 'abm_single_child', display: 'flex', position: 'absolute',
                    offsets: { top: 0, right: 20, bottom: 0, left: 10 },
                    width: 100, height: 50, color: C.d,
                    margin: { left: 'auto', right: 30, top: 'auto', bottom: 'auto' },
                }] }),
            ]),
        ],
    },
};

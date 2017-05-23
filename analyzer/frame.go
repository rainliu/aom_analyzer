package analyzer

import "bytes"

type FRAME struct {
	udc    ParameterSet
	cfh    ParameterSet
	width  int
	height int
	tiles  []*TILE
}

func clip(x, a, b int) int {
	if x < a {
		return a
	} else if x > b {
		return b
	} else {
		return x
	}
}

func fetch_pixels(pixels []string, width, height, tx, ty, tw int) string {
	var b bytes.Buffer
	for i := 0; i < tw; i++ {
		b.WriteString(pixels[clip(ty, 0, height-1)*width+clip(tx+i, 0, width-1)])
		b.WriteString(" ")
	}
	return b.String()
}

func (f *FRAME) Parse(loop, final []string) error {
	var loops, finals [3][]string
	loops[0] = loop[0 : f.width*f.height]
	loops[1] = loop[f.width*f.height : f.width*f.height+((f.width+1)/2)*((f.height+1)/2)]
	loops[2] = loop[f.width*f.height+((f.width+1)/2)*((f.height+1)/2):]

	finals[0] = final[0 : f.width*f.height]
	finals[1] = final[f.width*f.height : f.width*f.height+((f.width+1)/2)*((f.height+1)/2)]
	finals[2] = final[f.width*f.height+((f.width+1)/2)*((f.height+1)/2):]

	var w, h, x, y int
	for _, tile := range f.tiles {
		for _, lsb := range tile.lsbs {
			for _, sb := range lsb.sbs {
				for _, tb := range sb.tbs {
					lf := loops[tb.tx_plane]
					fn := finals[tb.tx_plane]
					if tb.tx_plane == 0 {
						w = f.width
						h = f.height
						x = sb.col_start + tb.tx_col
						y = sb.row_start + tb.tx_row
					} else {
						w = (f.width + 1) >> 1
						h = (f.height + 1) >> 1
						x = (sb.col_start >> 1) + tb.tx_col
						y = (sb.row_start >> 1) + tb.tx_row
					}
					for j := 0; j < tb.tx_size_high; j++ {
						line := fetch_pixels(lf, w, h, x, y, tb.tx_size_wide)
						tb.loop.Parse(line)

						line = fetch_pixels(fn, w, h, x, y, tb.tx_size_wide)
						tb.final.Parse(line)
					}
				}
			}
		}
	}

	return nil
}

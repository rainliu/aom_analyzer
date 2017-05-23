package analyzer

import (
	"fmt"
	"log"
	"strings"
)

const (
	STATE_PARSE_UDC = iota
	STATE_PARSE_CFH
	STATE_PARSE_TIL
	STATE_PARSE_LSB
	STATE_PARSE_SB
	STATE_PARSE_SBH
	STATE_PARSE_SBD
	STATE_PARSE_TOKEN
	STATE_PARSE_COEFF
	STATE_PARSE_RESID
	STATE_PARSE_PREDI
	STATE_PARSE_RECON
	STATE_PARSE_LOOP
	STATE_PARSE_FINAL
	STATE_PARSE_FRAME_DONE
)

func (a *Analyzer) ParseTrace() {
	var frame *FRAME
	var tileID int
	var lsbID int
	var sbID int
	var tbID int

	var loop, final []string

	state := STATE_PARSE_UDC
	for a.Scanner.Scan() {
		line := a.Scanner.Text()
		if strings.Contains(line, "Uncompressed Data Chunk") {
			state = STATE_PARSE_UDC
			if frame != nil {
				a.mutex.Lock()
				a.frames = append(a.frames, frame)
				a.mutex.Unlock()
			}
			frame = &FRAME{}
			loop = nil
			final = nil
		} else if strings.Contains(line, "Compressed Frame Header") {
			state = STATE_PARSE_CFH

			if _, err := fmt.Sscanf(line, "=====Compressed Frame Header (Frame Size %d %d)=========================",
				&frame.width, &frame.height); err != nil {
				fmt.Println(line)
				log.Fatal(err)
			}
		} else if strings.Contains(line, "Tile") {
			state = STATE_PARSE_TIL

			tile := &TILE{}
			if _, err := fmt.Sscanf(line, "=====Tile (TIL Info. %d %d <-> %d %d)===============================",
				&tile.col_start, &tile.row_start, &tile.col_end, &tile.row_end); err != nil {
				fmt.Println(line)
				log.Fatal(err)
			}
			frame.tiles = append(frame.tiles, tile)
			tileID = len(frame.tiles) - 1
		} else if strings.Contains(line, "Largest Super-Block") {
			state = STATE_PARSE_LSB

			lsb := &LSB{}
			if _, err := fmt.Sscanf(line, "=====Largest Super-Block (LSB Info. %d %d <-> %d %d)================",
				&lsb.col_start, &lsb.row_start, &lsb.col_end, &lsb.row_end); err != nil {
				fmt.Println(line)
				log.Fatal(err)
			}
			frame.tiles[tileID].lsbs = append(frame.tiles[tileID].lsbs, lsb)
			lsbID = len(frame.tiles[tileID].lsbs) - 1
		} else if strings.Contains(line, "Super-Block (") {
			state = STATE_PARSE_SB

			sb := &SB{}
			if _, err := fmt.Sscanf(line, "=====Super-Block (SB Pos. %d %d)========================================", &sb.col_start, &sb.row_start); err != nil {
				fmt.Println(line)
				log.Fatal(err)
			}
			frame.tiles[tileID].lsbs[lsbID].sbs = append(frame.tiles[tileID].lsbs[lsbID].sbs, sb)
			sbID = len(frame.tiles[tileID].lsbs[lsbID].sbs) - 1
		} else if strings.Contains(line, "Super-Block Head") {
			state = STATE_PARSE_SBH

			sb := frame.tiles[tileID].lsbs[lsbID].sbs[sbID]
			if _, err := fmt.Sscanf(line, "=====Super-Block Head (SBH Size %d %d)==================================", &sb.size_wide, &sb.size_high); err != nil {
				fmt.Println(line)
				log.Fatal(err)
			}
		} else if strings.Contains(line, "Super-Block Data") {
			state = STATE_PARSE_SBD
		} else if strings.Contains(line, "Predi") {
			state = STATE_PARSE_PREDI

			tb := &TB{}
			if _, err := fmt.Sscanf(line, "=====Predi (PLANE: %d, TX_POS: %d %d, TX_SIZE: %dx%d, TX_TYPE: %d)========",
				&tb.tx_plane, &tb.tx_col, &tb.tx_row, &tb.tx_size_wide, &tb.tx_size_high, &tb.tx_type); err != nil {
				fmt.Println(line)
				log.Fatal(err)
			}
			frame.tiles[tileID].lsbs[lsbID].sbs[sbID].tbs = append(frame.tiles[tileID].lsbs[lsbID].sbs[sbID].tbs, tb)
			tbID = len(frame.tiles[tileID].lsbs[lsbID].sbs[sbID].tbs) - 1
		} else if strings.Contains(line, "Token") {
			state = STATE_PARSE_TOKEN
		} else if strings.Contains(line, "Coeff") {
			state = STATE_PARSE_COEFF
		} else if strings.Contains(line, "Resid") {
			state = STATE_PARSE_RESID
		} else if strings.Contains(line, "Recon") {
			state = STATE_PARSE_RECON
		} else if strings.Contains(line, "Loop YUV") {
			state = STATE_PARSE_LOOP
		} else if strings.Contains(line, "Final YUV") {
			state = STATE_PARSE_FINAL
		} else if strings.Contains(line, "Frame Done") {
			state = STATE_PARSE_FRAME_DONE
			if err := frame.Parse(loop, final); err != nil {
				fmt.Println(line)
				log.Fatal(err)
			}
		} else {
			var err error
			switch state {
			case STATE_PARSE_UDC:
				err = frame.udc.Parse(line)
			case STATE_PARSE_CFH:
				err = frame.cfh.Parse(line)
			case STATE_PARSE_TIL:
			case STATE_PARSE_LSB:
			case STATE_PARSE_SB:
				err = frame.tiles[tileID].lsbs[lsbID].sbs[sbID].partition.Parse(line)
			case STATE_PARSE_SBH:
				err = frame.tiles[tileID].lsbs[lsbID].sbs[sbID].sbh.Parse(line)
			case STATE_PARSE_SBD:
				err = frame.tiles[tileID].lsbs[lsbID].sbs[sbID].sbd.Parse(line)
			case STATE_PARSE_TOKEN:
				err = frame.tiles[tileID].lsbs[lsbID].sbs[sbID].tbs[tbID].token.Parse(line)
			case STATE_PARSE_COEFF:
				err = frame.tiles[tileID].lsbs[lsbID].sbs[sbID].tbs[tbID].coef.Parse(line)
			case STATE_PARSE_RESID:
				err = frame.tiles[tileID].lsbs[lsbID].sbs[sbID].tbs[tbID].resi.Parse(line)
			case STATE_PARSE_PREDI:
				err = frame.tiles[tileID].lsbs[lsbID].sbs[sbID].tbs[tbID].pred.Parse(line)
			case STATE_PARSE_RECON:
				err = frame.tiles[tileID].lsbs[lsbID].sbs[sbID].tbs[tbID].reco.Parse(line)
			case STATE_PARSE_LOOP:
				pixels := strings.Split(strings.TrimSpace(line), " ")
				loop = append(loop, pixels...)
			case STATE_PARSE_FINAL:
				pixels := strings.Split(strings.TrimSpace(line), " ")
				final = append(final, pixels...)
			default:
			}
			if err != nil {
				log.Fatal(err)
			}
		}
	}

	a.Tracer.Wait()

	if frame != nil {
		a.mutex.Lock()
		a.frames = append(a.frames, frame)
		a.mutex.Unlock()
	}

	a.done = true
}

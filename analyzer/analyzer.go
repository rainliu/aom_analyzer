package analyzer

import (
	"bufio"
	"errors"
	"fmt"
	"os/exec"
	"strconv"
	"strings"
	"sync"
)

const Version = "20170505"

type Analyzer struct {
	Tracer  *exec.Cmd
	Scanner *bufio.Scanner

	done   bool
	mutex  sync.Mutex
	frames []*FRAME
}

const LSB_SIZE = 64
const MSB_SIZE = 4
const LOG2_LSB_SIZE = 6
const LOG2_MSB_SIZE = 2

const (
	CMD_FRAME = iota
	CMD_TILE
	CMD_LSB
	CMD_SB
	CMD_TB
)

var CmdStr2Id map[string]int

func init() {
	CmdStr2Id = make(map[string]int)
	CmdStr2Id["frame"] = CMD_FRAME
	CmdStr2Id["tile"] = CMD_TILE
	CmdStr2Id["lsb"] = CMD_LSB
	CmdStr2Id["sb"] = CMD_SB
	CmdStr2Id["tb"] = CMD_TB
}

func (a *Analyzer) ParseCmd(prefix string, reader *bufio.Reader) {
	cmdLevel := 0
	cmdID := 0

	for {
		fmt.Printf("%s>", prefix)
		line, err := reader.ReadString('\n')
		if err != nil {
			break
		}

		cmdString := strings.Fields(strings.TrimSpace(line))

		if len(cmdString) == 0 {
			continue
		} else if cmdString[0] == "exit" {
			break
		} else if cmdString[0] == "status" {
			if a.done {
				fmt.Printf("Tracer is finished: Total number of frame: %d are parsed\n", len(a.frames))
			} else {
				fmt.Printf("Tracer in progress: number of frame: %d are parsed by now\n", len(a.frames))
			}
		} else if cmdString[0] == "help" {
			a.Help(cmdString, cmdLevel, cmdID)
		} else if cmdString[0] == "frame" {
			if len(cmdString) == 1 {
				fmt.Printf("Total number of %s: %d\n", cmdString[0], len(a.frames))
			} else if len(cmdString) == 2 {
				if n, err := strconv.ParseInt(cmdString[1], 0, 0); err != nil {
					fmt.Printf("%s: Invalid Parameters \"%s\"\n", cmdString[0], cmdString[1])
				} else {
					if int(n) < len(a.frames) {
						frame := a.frames[n]
						a.ParseCmdFrame(prefix+"\\"+cmdString[0]+" "+cmdString[1], reader, cmdLevel+1, CmdStr2Id[cmdString[0]], frame)
					} else if a.done {
						fmt.Printf("Warning: can't find %d-th %s\n", n, cmdString[0])
					} else {
						fmt.Printf("Please wait AOM Analyzer to finish parsing %d-th %s\n", n, cmdString[0])
					}
				}
			} else {
				fmt.Printf("Too many parameters in command \"%s\"\n", cmdString[0])
			}
		} else {
			fmt.Printf("Unknown Command \"%s\"\n", cmdString[0])
		}
	}
}

func (a *Analyzer) ParseCmdFrame(prefix string, reader *bufio.Reader, cmdLevel int, cmdId int, frame *FRAME) {
	for {
		fmt.Printf("%s>", prefix)
		line, err := reader.ReadString('\n')
		if err != nil {
			break
		}

		cmdString := strings.Fields(strings.TrimSpace(line))

		if len(cmdString) == 0 {
			continue
		} else if cmdString[0] == "exit" {
			break
		} else if cmdString[0] == "status" {
			if a.done {
				fmt.Printf("Tracer is finished: Total number of frame: %d are parsed\n", len(a.frames))
			} else {
				fmt.Printf("Tracer in progress: number of frame: %d are parsed by now\n", len(a.frames))
			}
		} else if cmdString[0] == "help" {
			a.Help(cmdString, cmdLevel, cmdId)
		} else if cmdString[0] == "udc" {
			frame.udc.ShowInfo()
		} else if cmdString[0] == "cfh" {
			frame.cfh.ShowInfo()
		} else if cmdString[0] == "tile" ||
			cmdString[0] == "lsb" ||
			cmdString[0] == "sb" {
			if len(cmdString) == 1 {
				if cmdString[0] == "tile" {
					fmt.Printf("Total number of %s: %d\n", cmdString[0], len(frame.tiles))
				} else if cmdString[0] == "lsb" {
					n := 0
					for i := 0; i < len(frame.tiles); i++ {
						n += len(frame.tiles[i].lsbs)
					}
					fmt.Printf("Total Number of %s: %d\n", cmdString[0], n)
				} else { //"sb"
					n := 0
					for i := 0; i < len(frame.tiles); i++ {
						for j := 0; j < len(frame.tiles[i].lsbs); j++ {
							n += len(frame.tiles[i].lsbs[j].sbs)
						}
					}
					fmt.Printf("Total Number of %s: %d\n", cmdString[0], n)
				}
			} else if len(cmdString) == 2 {
				var n, x, y int64
				var input_xy bool

				var err error
				if strings.HasPrefix(cmdString[1], "(") && strings.HasSuffix(cmdString[1], ")") {
					xy := strings.Split(strings.TrimRight(strings.TrimLeft(cmdString[1], "("), ")"), ",")
					x, err = strconv.ParseInt(strings.TrimSpace(xy[0]), 0, 0)
					y, err = strconv.ParseInt(strings.TrimSpace(xy[1]), 0, 0)

					if int(x) < 0 || int(x) >= frame.width || int(y) < 0 || int(y) >= frame.height {
						err = errors.New("out of frame boundary")
					}
					input_xy = true
				} else {
					n, err = strconv.ParseInt(cmdString[1], 0, 0)
					input_xy = false
				}
				if err != nil {
					fmt.Printf("%s: Invalid Parameters \"%s\"\n", cmdString[0], cmdString[1])
				} else {
					found := false
					if input_xy {
						col := int(x)
						row := int(y)

						for i := 0; i < len(frame.tiles) && !found; i++ {
							if col >= frame.tiles[i].col_start &&
								row >= frame.tiles[i].row_start &&
								col < frame.tiles[i].col_end &&
								row < frame.tiles[i].row_end {
								if cmdString[0] == "tile" {
									a.ParseCmdTileLsbSb(prefix+"\\"+cmdString[0]+" ("+strconv.Itoa(frame.tiles[i].col_start)+","+strconv.Itoa(frame.tiles[i].row_start)+")", reader, cmdLevel+1, CmdStr2Id[cmdString[0]],
										frame, frame.tiles[i], nil, nil, nil)
									found = true
									break
								}

								for j := 0; j < len(frame.tiles[i].lsbs) && !found; j++ {
									if col >= frame.tiles[i].lsbs[j].col_start &&
										row >= frame.tiles[i].lsbs[j].row_start &&
										col < frame.tiles[i].lsbs[j].col_end &&
										row < frame.tiles[i].lsbs[j].row_end {
										if cmdString[0] == "lsb" {
											a.ParseCmdTileLsbSb(prefix+"\\"+cmdString[0]+" ("+strconv.Itoa(frame.tiles[i].lsbs[j].col_start)+","+strconv.Itoa(frame.tiles[i].lsbs[j].row_start)+")", reader, cmdLevel+1, CmdStr2Id[cmdString[0]],
												frame, frame.tiles[i], frame.tiles[i].lsbs[j], nil, nil)
											found = true
											break
										}

										for k := 0; k < len(frame.tiles[i].lsbs[j].sbs) && !found; k++ {
											if col >= frame.tiles[i].lsbs[j].sbs[k].col_start &&
												row >= frame.tiles[i].lsbs[j].sbs[k].row_start &&
												col < frame.tiles[i].lsbs[j].sbs[k].col_start+frame.tiles[i].lsbs[j].sbs[k].size_wide &&
												row < frame.tiles[i].lsbs[j].sbs[k].row_start+frame.tiles[i].lsbs[j].sbs[k].size_high {
												if cmdString[0] == "sb" {
													a.ParseCmdTileLsbSb(prefix+"\\"+cmdString[0]+" ("+strconv.Itoa(frame.tiles[i].lsbs[j].sbs[k].col_start)+","+strconv.Itoa(frame.tiles[i].lsbs[j].sbs[k].row_start)+")", reader, cmdLevel+1, CmdStr2Id[cmdString[0]],
														frame, frame.tiles[i], frame.tiles[i].lsbs[j], frame.tiles[i].lsbs[j].sbs[k], nil)
													found = true
													break
												}
											}
										}
									}
								}
							}
						}

						if !found {
							fmt.Printf("Warning: can't find %s that contains pixel(%d,%d)\n", cmdString[0], x, y)
						}
					} else {
						if cmdString[0] == "tile" {
							if int(n) < len(frame.tiles) {
								a.ParseCmdTileLsbSb(prefix+"\\"+cmdString[0]+" "+strconv.Itoa(int(n)), reader, cmdLevel+1, CmdStr2Id[cmdString[0]],
									frame, frame.tiles[int(n)], nil, nil, nil)
								found = true
							}
						} else if cmdString[0] == "lsb" {
							m := 0
							for i := 0; i < len(frame.tiles) && !found; i++ {
								if int(n) >= m && int(n) < m+len(frame.tiles[i].lsbs) {
									a.ParseCmdTileLsbSb(prefix+"\\"+cmdString[0]+" "+strconv.Itoa(int(n)), reader, cmdLevel+1, CmdStr2Id[cmdString[0]],
										frame, frame.tiles[i], frame.tiles[i].lsbs[int(n)-m], nil, nil)
									found = true
									break
								} else {
									m += len(frame.tiles[i].lsbs)
								}
							}
						} else if cmdString[0] == "sb" {
							m := 0
							for i := 0; i < len(frame.tiles) && !found; i++ {
								for j := 0; j < len(frame.tiles[i].lsbs) && !found; j++ {
									if int(n) >= m && int(n) < m+len(frame.tiles[i].lsbs[j].sbs) {
										a.ParseCmdTileLsbSb(prefix+"\\"+cmdString[0]+" "+strconv.Itoa(int(n)), reader, cmdLevel+1, CmdStr2Id[cmdString[0]],
											frame, frame.tiles[i], frame.tiles[i].lsbs[j], frame.tiles[i].lsbs[j].sbs[int(n)-m], nil)
										found = true
										break
									} else {
										m += len(frame.tiles[i].lsbs[j].sbs)
									}
								}
							}
						}

						if !found {
							fmt.Printf("Warning: can't find %d-th %s\n", n, cmdString[0])
						}
					}
				}
			} else {
				fmt.Printf("Too many parameters in command \"%s\"\n", cmdString[0])
			}
		} else {
			fmt.Printf("Unknown command \"%s\"\n", cmdString[0])
		}
	}
}

func (a *Analyzer) ParseCmdTileLsbSb(prefix string, reader *bufio.Reader, cmdLevel int, cmdId int, frame *FRAME, tile *TILE, lsb *LSB, sb *SB, tb *TB) {
	for {
		fmt.Printf("%s>", prefix)
		line, err := reader.ReadString('\n')
		if err != nil {
			break
		}

		cmdString := strings.Fields(strings.TrimSpace(line))

		if len(cmdString) == 0 {
			continue
		} else if cmdString[0] == "exit" {
			break
		} else if cmdString[0] == "status" {
			if a.done {
				fmt.Printf("Tracer is finished: Total number of frame: %d are parsed\n", len(a.frames))
			} else {
				fmt.Printf("Tracer in progress: number of frame: %d are parsed by now\n", len(a.frames))
			}
		} else if cmdString[0] == "help" {
			a.Help(cmdString, cmdLevel, cmdId)
		} else if (cmdString[0] == "tb") && (cmdId == CMD_SB || cmdId == CMD_TB) {
			if len(cmdString) == 1 {
				if cmdLevel == 2 {
					fmt.Printf("Total number of %s: %d\n", cmdString[0], len(sb.tbs))
				} else {
					tb.ShowInfo()
				}
			} else if len(cmdString) == 2 {
				var n, x, y, z int64
				var input_xyz bool

				var err error
				if strings.HasPrefix(cmdString[1], "(") && strings.HasSuffix(cmdString[1], ")") {
					xyz := strings.Split(strings.TrimRight(strings.TrimLeft(cmdString[1], "("), ")"), ",")
					x, err = strconv.ParseInt(strings.TrimSpace(xyz[0]), 0, 0)
					y, err = strconv.ParseInt(strings.TrimSpace(xyz[1]), 0, 0)
					z, err = strconv.ParseInt(strings.TrimSpace(xyz[2]), 0, 0)
					if int(x) < 0 || int(x) >= sb.size_wide || int(y) < 0 || int(y) >= sb.size_high || int(z) >= 3 {
						err = errors.New("out of sb boundary or color space")
					}
					input_xyz = true
				} else {
					n, err = strconv.ParseInt(cmdString[1], 0, 0)
					input_xyz = false
				}
				if err != nil {
					fmt.Printf("%s: Invalid Parameters \"%s\" due to %s\n", cmdString[0], cmdString[1], err.Error())
				} else {
					found := false
					if input_xyz {
						tx_col := int(x)
						tx_row := int(y)
						tx_plane := int(z)
						for i := 0; i < len(sb.tbs) && !found; i++ {
							if tx_plane == sb.tbs[i].tx_plane &&
								tx_col >= sb.tbs[i].tx_col &&
								tx_row >= sb.tbs[i].tx_row &&
								tx_col < sb.tbs[i].tx_col+sb.tbs[i].tx_size_wide &&
								tx_row < sb.tbs[i].tx_row+sb.tbs[i].tx_size_high {
								a.ParseCmdTileLsbSb(prefix+"\\"+cmdString[0]+" ("+strconv.Itoa(sb.tbs[i].tx_col)+","+strconv.Itoa(sb.tbs[i].tx_row)+","+strconv.Itoa(tx_plane)+")", reader, cmdLevel+1, CmdStr2Id[cmdString[0]],
									frame, tile, lsb, sb, sb.tbs[i])
								found = true
								break
							}
						}

						if !found {
							fmt.Printf("Warning: can't find %d-th plane tb that contains relative pixel(%d,%d) inside sb\n", z, x, y)
						}
					} else {
						if int(n) < len(sb.tbs) {
							a.ParseCmdTileLsbSb(prefix+"\\"+cmdString[0]+" "+strconv.Itoa(int(n)), reader, cmdLevel+1, CmdStr2Id[cmdString[0]],
								frame, tile, lsb, sb, sb.tbs[n])
							found = true
						}

						if !found {
							fmt.Printf("Warning: can't find %d-th %s\n", n, cmdString[0])
						}
					}
				}
			}
		} else if len(cmdString) != 1 {
			fmt.Printf("Too many parameters in command \"%s\"\n", cmdString[0])
		} else if cmdString[0] == "udc" {
			frame.udc.ShowInfo()
		} else if cmdString[0] == "cfh" {
			frame.cfh.ShowInfo()
		} else if cmdString[0] == "tile" {
			tile.ShowInfo()
		} else if cmdString[0] == "lsb" && (cmdId == CMD_LSB || cmdId == CMD_SB || cmdId == CMD_TB) {
			lsb.ShowInfo()
		} else if (cmdString[0] == "sb" ||
			cmdString[0] == "sbh" ||
			cmdString[0] == "sbd" ||
			cmdString[0] == "tb") &&
			(cmdId == CMD_SB || cmdId == CMD_TB) {
			if cmdString[0] == "sb" {
				sb.ShowInfo()
			} else if cmdString[0] == "sbh" {
				sb.sbh.ShowInfo()
			} else if cmdString[0] == "sbd" {
				sb.sbd.ShowInfo()
			}
		} else if (cmdString[0] == "token" ||
			cmdString[0] == "coef" ||
			cmdString[0] == "resi" ||
			cmdString[0] == "pred" ||
			cmdString[0] == "reco" ||
			cmdString[0] == "loop" ||
			cmdString[0] == "final") && cmdId == CMD_TB {
			if cmdString[0] == "token" {
				tb.token.ShowInfo()
			} else if cmdString[0] == "coef" {
				tb.coef.ShowInfo()
			} else if cmdString[0] == "resi" {
				tb.resi.ShowInfo()
			} else if cmdString[0] == "pred" {
				tb.pred.ShowInfo()
			} else if cmdString[0] == "reco" {
				tb.reco.ShowInfo()
			} else if cmdString[0] == "loop" {
				tb.loop.ShowInfo()
			} else if cmdString[0] == "final" {
				tb.final.ShowInfo()
			}
		} else {
			fmt.Printf("Unknown command \"%s\"\n", cmdString[0])
		}
	}
}

package analyzer

import (
	"fmt"
)

func (a *Analyzer) Perform(cmdString []string, cmdLevel int, cmdId int) {
	if cmdLevel == 0 { //Sequence-Level
		switch cmdString[1] {
		case "exit":
			fmt.Printf("exit: exit AOM Analyzer\n")
		case "status":
			fmt.Printf("status: show current AOM trace parsing complete status\n")
		default:
			fmt.Printf("frame [n]: show num of frame [or goto n-th (decoding order) frame]\n") //frame
		}
	} else if cmdLevel == 1 { //Frame-Level
		switch cmdString[1] {
		case "exit":
			fmt.Printf("exit: back to up-level\n")
		case "status":
			fmt.Printf("status: show current AOM trace parsing complete status\n")
		case "udc":
			fmt.Printf("udc: show current frame's uncompressed data chunk info\n")
		case "cfh":
			fmt.Printf("cfh: show current frame's compressed frame header info\n")
		case "tile":
			fmt.Printf("tile [n|(x,y)]: show num of tile [or goto n-th tile or tile contains pixel(x,y)]\n")
		case "lsb":
			fmt.Printf("lsb [n|(x,y)]: show num of lsb [or goto n-th lsb or lsb that contains pixel(x,y)]\n")
		default:
			fmt.Printf("sb [n|(x,y)]: show num of sb [or goto n-th sb or sb that contains pixel(x,y)]\n")
		}
	} else if cmdLevel == 2 {
		if cmdId == CMD_TILE { //Tile-Level
			switch cmdString[1] {
			case "exit":
				fmt.Printf("exit: back to up-level\n")
			case "status":
				fmt.Printf("status: show current AOM trace parsing complete status\n")
			case "udc":
				fmt.Printf("udc: show current tile's uncompressed data chunk info\n")
			case "cfh":
				fmt.Printf("cfh: show current tile's compressed frame header info\n")
			default:
				fmt.Printf("tile: show current tile info\n")
			}
		} else if cmdId == CMD_LSB { //LSB-level
			switch cmdString[1] {
			case "exit":
				fmt.Printf("exit: back to up-level\n")
			case "status":
				fmt.Printf("status: show current AOM trace parsing complete status\n")
			case "udc":
				fmt.Printf("udc: show current lsb's uncompressed data chunk info\n")
			case "cfh":
				fmt.Printf("cfh: show current lsb's compressed frame header info\n")
			case "tile":
				fmt.Printf("tile: show current lsb's tile info\n")
			default:
				fmt.Printf("lsb: show current lsb info\n")
			}
		} else if cmdId == CMD_SB { //SB-level
			switch cmdString[1] {
			case "exit":
				fmt.Printf("exit: back to up-level\n")
			case "status":
				fmt.Printf("status: show current AOM trace parsing complete status\n")
			case "udc":
				fmt.Printf("udc: show current sb's uncompressed data chunk info\n")
			case "cfh":
				fmt.Printf("cfh: show current sb's compressed frame header info\n")
			case "tile":
				fmt.Printf("tile: show current sb's tile info\n")
			case "lsb":
				fmt.Printf("lsb: show current sb's lsb info\n")
			case "sb":
				fmt.Printf("sbh: show current sb info\n")
			case "sbh":
				fmt.Printf("sbh: show current sb's head info\n")
			case "sbd":
				fmt.Printf("shd: show current sb's data info\n")
			default:
				fmt.Printf("tb [n|(x,y,z)]: show num of tb [or goto n-th tb or z-th plane tb that contains relative pixel(x,y) inside sb]\n")
			}
		}
	} else if cmdLevel == 3 {
		if cmdId == CMD_TB { //TB-level
			switch cmdString[1] {
			case "exit":
				fmt.Printf("exit: back to up-level\n")
			case "status":
				fmt.Printf("status: show current AOM trace parsing complete status\n")
			case "udc":
				fmt.Printf("udc: show current tb's uncompressed data chunk info\n")
			case "cfh":
				fmt.Printf("cfh: show current tb's compressed frame header info\n")
			case "tile":
				fmt.Printf("tile: show current tb's tile info\n")
			case "lsb":
				fmt.Printf("lsb: show current tb's lsb info\n")
			case "sb":
				fmt.Printf("sbh: show current tb info\n")
			case "sbh":
				fmt.Printf("sbh: show current tb's sb head info\n")
			case "sbd":
				fmt.Printf("shd: show current tb's sb data info\n")
			case "tb":
				fmt.Printf("tb: show current tb's info\n")
			case "token":
				fmt.Printf("token: show current tb's token info\n")
			case "coef":
				fmt.Printf("coef: show current tb's coefficient pixels\n")
			case "resi":
				fmt.Printf("resi: show current tb's residual pixels\n")
			case "pred":
				fmt.Printf("pred: show current tb's prediction pixels\n")
			case "reco":
				fmt.Printf("reco: show current tb's reconstruction pixels\n")
			case "loop":
				fmt.Printf("loop: show current tb's deblocking pixels\n")
			default:
				fmt.Printf("final: show current tb's final pixels\n")
			}
		}
	}
}

func (a *Analyzer) Help(cmdString []string, cmdLevel int, cmdId int) {
	switch len(cmdString) {
	case 1: //help only
		if cmdLevel == 0 {
			fmt.Printf("help [exit|status|frame]\n")
		} else if cmdLevel == 1 {
			fmt.Printf("help [exit|status|udc|cfh|tile|lsb|sb]\n")
		} else if cmdLevel == 2 {
			if cmdId == CMD_TILE {
				fmt.Printf("help [exit|status|udc|cfh|tile]\n")
			} else if cmdId == CMD_LSB {
				fmt.Printf("help [exit|status|udc|cfh|tile|lsb]\n")
			} else if cmdId == CMD_SB {
				fmt.Printf("help [exit|status|udc|cfh|tile|lsb|sb|sbh|sbd|tb]\n")
			}
		} else if cmdLevel == 3 {
			if cmdId == CMD_TB {
				fmt.Printf("help [exit|status|udc|cfh|tile|lsb|sb|sbh|sbd|tb|token|coef|resi|pred|reco|loop|final]\n")
			}
		}
	case 2: //help + command
		if cmdLevel == 0 {
			if cmdString[1] == "exit" ||
				cmdString[1] == "status" ||
				cmdString[1] == "frame" {
				a.Perform(cmdString, cmdLevel, cmdId)
			} else {
				fmt.Printf("%s: Unknown Parameters \"%s\"\n", cmdString[0], cmdString[1])
				fmt.Printf("help [exit|status|frame]\n")
			}
		} else if cmdLevel == 1 {
			if cmdString[1] == "exit" ||
				cmdString[1] == "status" ||
				cmdString[1] == "tile" ||
				cmdString[1] == "lsb" ||
				cmdString[1] == "sb" {
				a.Perform(cmdString, cmdLevel, cmdId)
			} else {
				fmt.Printf("%s: Unknown Parameters \"%s\"\n", cmdString[0], cmdString[1])
				fmt.Printf("help [exit|status|tile|lsb|sb]\n")
			}
		} else if cmdLevel == 2 {
			if cmdId == CMD_TILE {
				if cmdString[1] == "exit" ||
					cmdString[1] == "status" ||
					cmdString[1] == "udc" ||
					cmdString[1] == "cfh" ||
					cmdString[1] == "tile" {
					a.Perform(cmdString, cmdLevel, cmdId)
				} else {
					fmt.Printf("%s: Unknown Parameters \"%s\"\n", cmdString[0], cmdString[1])
					fmt.Printf("help [exit|status|udc|cfh|tile]\n")
				}
			} else if cmdId == CMD_LSB {
				if cmdString[1] == "exit" ||
					cmdString[1] == "status" ||
					cmdString[1] == "udc" ||
					cmdString[1] == "cfh" ||
					cmdString[1] == "tile" ||
					cmdString[1] == "lsb" {
					a.Perform(cmdString, cmdLevel, cmdId)
				} else {
					fmt.Printf("%s: Unknown Parameters \"%s\"\n", cmdString[0], cmdString[1])
					fmt.Printf("help [exit|status|udc|cfh|tile|lsb]\n")
				}
			} else if cmdId == CMD_SB {
				if cmdString[1] == "exit" ||
					cmdString[1] == "status" ||
					cmdString[1] == "udc" ||
					cmdString[1] == "cfh" ||
					cmdString[1] == "tile" ||
					cmdString[1] == "lsb" ||
					cmdString[1] == "sb" ||
					cmdString[1] == "sbh" ||
					cmdString[1] == "sbd" ||
					cmdString[1] == "tb" {
					a.Perform(cmdString, cmdLevel, cmdId)
				} else {
					fmt.Printf("%s: Unknown Parameters \"%s\"\n", cmdString[0], cmdString[1])
					fmt.Printf("help [exit|status|udc|cfh|tile|lsb|sbh|sbd|tb]\n")
				}
			}
		} else if cmdLevel == 3 {
			if cmdId == CMD_TB {
				if cmdString[1] == "exit" ||
					cmdString[1] == "status" ||
					cmdString[1] == "udc" ||
					cmdString[1] == "cfh" ||
					cmdString[1] == "tile" ||
					cmdString[1] == "lsb" ||
					cmdString[1] == "sb" ||
					cmdString[1] == "sbh" ||
					cmdString[1] == "sbd" ||
					cmdString[1] == "tb" ||
					cmdString[1] == "token" ||
					cmdString[1] == "coef" ||
					cmdString[1] == "resi" ||
					cmdString[1] == "pred" ||
					cmdString[1] == "reco" ||
					cmdString[1] == "loop" ||
					cmdString[1] == "final" {
					a.Perform(cmdString, cmdLevel, cmdId)
				} else {
					fmt.Printf("%s: Unknown Parameters \"%s\"\n", cmdString[0], cmdString[1])
					fmt.Printf("help [exit|status|udc|cfh|tile|lsb|sbh|sbd|tb|token|coef|resi|pred|reco|loop|final]\n")
				}
			}
		}
	default:
		fmt.Printf("Too many parameters in \"%s\"\n", cmdString[0])
	}
}

AOM_ANALYZER
============

1). cd aom_analyzer/aom

2). open aom.sln in Visual Studio 2013 to build aomdec.exe to aom_analyzer/aom/bin/

3). cd aom_analyzer/

4). run "go build" to get aom_analyzer.exe

5). run "./aom_analyzer.exe bitstream.webm"

6). type 'help' for more commands information


Command Help:
============

1). Sequence-Level: (exit|status|frame)

1.1) exit: exit AOM Analyzer

1.2) status: show current AOM trace parsing complete status

1.3) frame [n]: show num of frame [or goto n-th (decoding order) frame]

2). Frame-Level: (exit|status|udc|cfh|tile|lsb|sb)

2.1) exit: back to up-level

2.2) status: show current AOM trace parsing complete status

2.3) udc: show current frame's uncompressed data chunk info

2.4) cfh: show current frame's compressed frame header info

2.5) tile [n|(x,y)]: show num of tile [or goto n-th tile or tile contains pixel(x,y)]

2.6) lsb [n|(x,y)]: show num of lsb [or goto n-th lsb or lsb that contains pixel(x,y)]

2.7) sb [n|(x,y)]: show num of sb [or goto n-th sb or sb that contains pixel(x,y)]

3). Tile-Level: (exit|status|udc|cfh|tile)

3.1) exit: back to up-level

3.2) status: show current AOM trace parsing complete status

3.3) udc: show current tile's uncompressed data chunk info

3.4) cfh: show current tile's compressed frame header info

3.5) tile: show current tile info

4). LSB-Level: (exit|status|udc|cfh|tile|lsb)

4.1) exit: back to up-level

4.2) status: show current AOM trace parsing complete status

4.3) udc: show current lsb's uncompressed data chunk info

4.4) cfh: show current lsb's compressed frame header info

4.5) tile: show current lsb's tile info

4.6) lsb: show current lsb info

5). SB-Level: (exit|status|udc|cfh|tile|lsb|sb|sbh|sbd|tb)

5.1) exit: back to up-level

5.2) status: show current AOM trace parsing complete status

5.3) udc: show current sb's uncompressed data chunk info

5.4) cfh: show current sb's compressed frame header info

5.5) tile: show current sb's tile info

5.6) lsb: show current sb's lsb info

5.7) sb: show current sb info

5.8) sbh: show current sb's header info

5.9) sbd: show current sb's data info

5.10) tb [n|(x,y,z)]: show num of tb [or goto n-th tb or z-th plane tb that contains relative pixel(x,y) inside sb]

6). TB-Level: (exit|status|udc|cfh|tile|lsb|sb|sbh|sbd|tb|token|coef|resi|pred|reco|loop|final)

6.1) exit: back to up-level

6.2) status: show current AOM trace parsing complete status

6.3) udc: show current tb's uncompressed data chunk info

6.4) cfh: show current tb's compressed frame header info

6.5) tile: show current tb's tile info

6.6) lsb: show current tb's lsb info

6.7) sb: show current tb's sb info

6.8) sbh: show current tb's sb header info

6.9) sbd: show current tb's sb data info

6.10) tb: show num of tb info

6.11) token: show current tb's token info

6.12) coef: show current tb's coefficient pixels

6.13) resi: show current tb's residual pixels

6.14) pred: show current tb's prediction pixels

6.15) reco: show current tb's reconstruction pixels

6.16) loop: show current tb's deblocking pixels

6.17) final: show current tb's final pixels
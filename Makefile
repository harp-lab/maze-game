




default:
	clang++ -v -O2 -o server maze-game-server.cpp -ferror-limit=2 -L /usr/local/lib/libGraphicsMagick++.a `GraphicsMagick++-config --cppflags --cxxflags --ldflags --libs`

debug:
	clang++ -v -g -o server maze-game-server.cpp -ferror-limit=2 -L /usr/local/lib/libGraphicsMagick++.a `GraphicsMagick++-config --cppflags --cxxflags --ldflags --libs`

dfsbot:
	make default && ./server 'python3 dfsbot.py'

randobot:
	make default && ./server 'python3 randobot.py'

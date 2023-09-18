




default:
	clang++ -v -o server maze-game-server.cpp -L /usr/local/lib/libGraphicsMagick++.a `GraphicsMagick++-config --cppflags --cxxflags --ldflags --libs`
dfsbot:
	make default && ./server 'python3 dfsbot.py'

QTDIR=/opt/qt5
test: test.cpp Makefile
	${CXX} -std=c++11 -O3 -march=native -mfpmath=sse -g test.cpp -o test
visual: visual.cpp Makefile
	${CXX} -std=c++11 -O3 -march=native -g -fPIC visual.cpp -o visual -I${QTDIR}/include -I${QTDIR}/include/QtGui -L${QTDIR}/lib -lQt5Core -lQt5Gui

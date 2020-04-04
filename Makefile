QTDIR=/opt/qt5
visual: visual.cpp Makefile
	${CXX} -std=c++11 -O3 -march=native -g -fPIC visual.cpp -o visual -I${QTDIR}/include -I${QTDIR}/include/QtGui -L${QTDIR}/lib -lQt5Core -lQt5Gui ${CXXFLAGS} ${LDFLAGS}

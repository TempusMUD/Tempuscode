UTILS=hcontrol-view showplay economic_stats demographic_stats scan_rent purgeplay
INCLUDE=-I../../src/include
CXX=g++ -Wall -O3 $(INCLUDE)

all: $(UTILS)

hcontrol-view: hcontrol-view.cc
	$(CXX) hcontrol-view.cc -o $@

showplay: showplay.cc
	$(CXX) showplay.cc -o $@

economic_stats: economic_stats.cc
	$(CXX) economic_stats.cc -o $@

demographic_stats: demographic_stats.cc
	$(CXX) demographic_stats.cc -o $@

scan_rent: scan_rent.cc
	$(CXX) scan_rent.cc -o $@

purgeplay: purgeplay.cc
	$(CXX) purgeplay.cc -o $@
UTILS=hcontrol-view showplay economic_stats demographic_stats
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


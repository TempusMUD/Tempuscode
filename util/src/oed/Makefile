UTILS=#oed rview


default all: $(UTILS)

$(UTILS): 
	$(CXX) -o bin/$@ src/$@.cc

clean:     
	@printf "Cleaning\n" $@ $(TARGET)
	@for file in $(UTILS); do\
		rm -f bin/$$file;\
	done;


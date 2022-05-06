CXX=g++

CXXFLAGS=-std=c++11 -g

pagingwithtlb : pagingwithtlb.o level.o pagetable.o tracereader.o output_mode_helpers.o Map.o
	$(CXX) $(CXXFLAGS) -o pagingwithtlb $^

pagingwithtlb.o : pagingwithtlb.cpp pagetable.h level.h Map.h output_mode_helpers.h tracereader.h

pagetable.o : pagetable.cpp pagetable.h level.h Map.h 

level.o : level.cpp level.h pagetable.h Map.h

output_mode_helpers.o : output_mode_helpers.h output_mode_helpers.cpp

tracereader.o : tracereader.h tracereader.cpp

Map.o : Map.h Map.cpp

clean : 
	rm *.o
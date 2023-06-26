compile: run_5stage run_5stage_bypass 

run_5stage: 5stage.cpp 5stage.hpp
	g++ 5stage.cpp 5stage.hpp -o run_5stage
	
run_5stage_bypass: 5stage_bypass.cpp 5stage_bypass.hpp
	g++ 5stage_bypass.cpp 5stage_bypass.hpp -o run_5stage_bypass




clean:
	rm -f run_5stage run_5stage_bypass 

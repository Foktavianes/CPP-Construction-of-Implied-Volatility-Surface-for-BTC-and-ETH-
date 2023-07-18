step1: 	step1.cpp CsvFeeder.cpp CsvFeeder.h Msg.h
	g++ -o bin/step1.out step1.cpp CsvFeeder.cpp -std=c++17
step2: 	step1.cpp CsvFeeder.cpp CsvFeeder.h Msg.h VolSurfBuilder.h
	g++ -o bin/step2.out step2.cpp CsvFeeder.cpp -std=c++17
# step 3's compilation rule could be more complicated since it involves external libraries,
# the simple rule below only works if we do not change anything under Solver/, otherwise we have to list the dependencies, or use CMake
step3: 	step1.cpp CsvFeeder.cpp CsvFeeder.h Msg.h VolSurfBuilder.h CubicSmile.cpp CubicSmile.h Date.cpp
	g++ -o bin/step3.out step3.cpp CsvFeeder.cpp CubicSmile.cpp Date.cpp -std=c++17 -ISolver

#### some test programs
test_lbfgs: Solver/LBFGSpp/examples/example-rosenbrock-box.cpp
	g++ -o bin/test_lbfgs Solver/LBFGSpp/examples/example-rosenbrock-box.cpp -ISolver
test_ts_parser: test_ts_parser.cpp CsvFeeder.cpp
	g++ -o bin/test_ts_parser test_ts_parser.cpp CsvFeeder.cpp -std=c++17

clean:
	rm ./bin/*

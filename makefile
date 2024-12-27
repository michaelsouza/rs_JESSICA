black:
	black --line-length 120 codes/bbsolver_patterns.py

run_python: black
	clear; python codes/bbsolver_patterns.py

pgen_git:
	python ../prompt-generator/pgen.py prompt.yml --gitdiff

run_cpp:
	cd epanet-dev/build/bin/
	./run-epanet3
	
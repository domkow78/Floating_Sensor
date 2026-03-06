idf_component_register(SRCS 
	src/Argument.cpp 
	src/Command.cpp 
	src/CommandError.cpp 
	src/SimpleCLI.cpp 
	src/c/arg.c 
	src/c/cmd.c
	src/c/cmd_error.c
	src/c/comparator.c
	src/c/parser.c
                    INCLUDE_DIRS "src" 
                    PRIV_INCLUDE_DIRS "src/c"
                    )
                    
#                    REQUIRES arduino
                    
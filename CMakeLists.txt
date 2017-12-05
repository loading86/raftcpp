SET (LIBRAFT_SRC
	raftpb/raft.pb.cc
	utilies.cpp
	logger.cpp
	log_unstable.cpp
	storage.cpp
	log.cpp
)

INCLUDE_DIRECTORIES(./raftpb)
INCLUDE_DIRECTORIES(./include)
add_compile_options(-std=c++11)
ADD_LIBRARY (raft STATIC ${LIBRAFT_SRC})

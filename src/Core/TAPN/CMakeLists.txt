


add_library(TAPN InhibitorArc.cpp TimedArcPetriNet.cpp
TimedPlace.cpp TimeInterval.cpp TransportArc.cpp
OutputArc.cpp TimedInputArc.cpp TimedTransition.cpp TimeInvariant.cpp
TAPNModelBuilder.cpp StochasticStructure.cpp WatchExpression.cpp)
target_link_libraries(TAPN PlaceVisitor)
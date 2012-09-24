/*
 * SuccessorGenerator.cpp
 *
 *  Created on: 22/03/2012
 *      Author: MathiasGS
 */

#include "TimeDartSuccessorGenerator.hpp"

namespace VerifyTAPN {
namespace DiscreteVerification {

TimeDartSuccessorGenerator::~TimeDartSuccessorGenerator(){

}

TimeDartSuccessorGenerator::TimeDartSuccessorGenerator(TAPN::TimedArcPetriNet& tapn)  : tapn(tapn), allwaysEnabled(), numberoftransitions(tapn.GetTransitions().size()), transitionStatistics(){
	//Find the transitions which don't have input arcs
	transitionStatistics = new unsigned int [numberoftransitions];
	ClearTransitionsArray();
	for(TimedTransition::Vector::const_iterator iter = tapn.GetTransitions().begin(); iter != tapn.GetTransitions().end(); iter++){
		if((*iter)->GetPreset().size() + (*iter)->GetTransportArcs().size() == 0){
			allwaysEnabled.push_back(iter->get());
		}
	}
}

vector< NonStrictMarking > TimeDartSuccessorGenerator::generateSuccessors(const NonStrictMarking& marking, const TimedTransition& transition) const{
	vector< NonStrictMarking > result;
	ArcHashMap enabledArcs(transition.GetPresetSize() + transition.GetTransportArcs().size());

	// Calculate enabling tokens
	for(TAPN::TimedInputArc::WeakPtrVector::const_iterator arc_iter = transition.GetPreset().begin();
			arc_iter != transition.GetPreset().end(); arc_iter++){
			processArc(enabledArcs,	marking.GetTokenList( arc_iter->lock()->InputPlace().GetIndex() ), arc_iter->lock()->Interval(), arc_iter->lock().get(), transition);
	}

	for(TAPN::TransportArc::WeakPtrVector::const_iterator arc_iter = transition.GetTransportArcs().begin();
			arc_iter != transition.GetTransportArcs().end(); arc_iter++){
			processArc(enabledArcs,	marking.GetTokenList( arc_iter->lock()->Source().GetIndex() ), arc_iter->lock()->Interval(), arc_iter->lock().get(), transition, arc_iter->lock()->Destination().GetInvariant().GetBound());
	}

	generateMarkings(result, marking, transition, enabledArcs);
	return result;
}

void TimeDartSuccessorGenerator::processArc(
		ArcHashMap& enabledArcs,
		const TokenList& tokens,
		const TAPN::TimeInterval& interval,
		const void* arcAddress,
		const TimedTransition& transition,
		int bound
) const{
	for(TokenList::const_iterator token_iter = tokens.begin();	token_iter != tokens.end(); token_iter++){
		if(interval.GetLowerBound() <= token_iter->getAge() && token_iter->getAge() <= interval.GetUpperBound() && token_iter->getAge() <= bound){
			enabledArcs[arcAddress].push_back(*token_iter);
		}
	}
}


TokenList TimeDartSuccessorGenerator::getPlaceFromMarking(const NonStrictMarking& marking, int placeID) const{
	for(PlaceList::const_iterator iter = marking.GetPlaceList().begin();
			iter != marking.GetPlaceList().end();
			iter++){
		if(iter->place->GetIndex() == placeID) return iter->tokens;
	}
	return TokenList();
}

void TimeDartSuccessorGenerator::generateMarkings(vector<NonStrictMarking>& result, const NonStrictMarking& init_marking,
		const TimedTransition& transition, ArcHashMap& enabledArcs) const {

		bool inhibited = false;
		//Check that no inhibitors is enabled;

		for(TAPN::InhibitorArc::WeakPtrVector::const_iterator inhib_iter = transition.GetInhibitorArcs().begin(); inhib_iter != transition.GetInhibitorArcs().end(); inhib_iter++){
			// Maybe this could be done more efficiently using ArcHashMap? Dunno exactly how it works
			if(init_marking.NumberOfTokensInPlace(inhib_iter->lock().get()->InputPlace().GetIndex()) >= inhib_iter->lock().get()->GetWeight()){
				inhibited = true;
				break;
			}
		}
		if (inhibited) return;

		NonStrictMarking m(init_marking);
		m.SetGeneratedBy(&transition);
		//Generate markings for transition
		recursiveGenerateMarking(result, m, transition, 0, enabledArcs);
}


void TimeDartSuccessorGenerator::recursiveGenerateMarking(vector<NonStrictMarking>& result, NonStrictMarking& init_marking, const TimedTransition& transition, unsigned int index, ArcHashMap& enabledArcs) const{

	// Initialize vectors
	ArcAndTokensVector indicesOfCurrentPermutation;
	for(TimedInputArc::WeakPtrVector::const_iterator iter = transition.GetPreset().begin(); iter != transition.GetPreset().end(); iter++){
		InputArcAndTokens* arcAndTokens = new InputArcAndTokens(*iter, enabledArcs[iter->lock().get()]);
		if(arcAndTokens->isOK){
			indicesOfCurrentPermutation.push_back(arcAndTokens);
		}else{
			return;
		}
	}
	// Transport arcs
	for(TransportArc::WeakPtrVector::const_iterator iter = transition.GetTransportArcs().begin(); iter != transition.GetTransportArcs().end(); iter++){
		TransportArcAndTokens* arcAndTokens = new TransportArcAndTokens(*iter, enabledArcs[iter->lock().get()]);
		if(arcAndTokens->isOK){
			indicesOfCurrentPermutation.push_back(arcAndTokens);
		}else{
			return;
		}
	}

	// Write statistics
	transitionStatistics[transition.GetIndex()]++;

	// Generate permutations
	bool changedSomething = true;
	while(changedSomething){
		changedSomething = false;
		addMarking(result, init_marking, transition, indicesOfCurrentPermutation);

		//Loop through arc indexes from the back
		for(int arcAndTokenIndex = indicesOfCurrentPermutation.size()-1; arcAndTokenIndex >= 0; arcAndTokenIndex--){
			TokenList& enabledTokens = indicesOfCurrentPermutation.at(arcAndTokenIndex).enabledBy;
			vector<unsigned int >& modificationVector = indicesOfCurrentPermutation.at(arcAndTokenIndex).modificationVector;
			if(incrementModificationVector(modificationVector, enabledTokens)){
				changedSomething = true;
				break;
			}
		}
	}
}

bool TimeDartSuccessorGenerator::incrementModificationVector(vector<unsigned int >& modificationVector, TokenList& enabledTokens) const{
	unsigned int numOfTokenIndices = enabledTokens.size();

	unsigned int refrences[numOfTokenIndices];

	for(unsigned int i = 0; i < enabledTokens.size(); i++){
		refrences[i] = enabledTokens[i].getCount();
	}

	for(unsigned int i = 0; i < modificationVector.size(); i++){
		refrences[modificationVector[i]]--;
	}

	int modificationVectorSize = modificationVector.size();

	vector<unsigned int> tmp = modificationVector;
	// Loop through modification vector from the back
	for(int i = modificationVectorSize-1; i >= 0; i--){

		//Possible to increment index
		if(modificationVector[i] < numOfTokenIndices-1){
			//Increment index
			refrences[modificationVector.at(i)]++;
			modificationVector.at(i)++;
			refrences[modificationVector.at(i)]--;

			// Fix following indexes
			if(i < modificationVectorSize-1){
				unsigned int toSet = modificationVector.at(i);

				for(i++; i < modificationVectorSize; i++){
					//Find next index to set (die if not possible)
					while(refrences[toSet] == 0){
						toSet++;
						if(toSet >= numOfTokenIndices){
							modificationVector = tmp;
							return false;
						}
					}

					//Set index
					refrences[toSet]--;
					modificationVector[i] = toSet;
				}
			}
			return true;
		}else{
			// Free index
			refrences[modificationVector[i]]++;
			// Change index
			i--;
		}
	}
	modificationVector = tmp;
	return false;
}

void TimeDartSuccessorGenerator::addMarking(vector<NonStrictMarking >& result, NonStrictMarking& init_marking, const TimedTransition& transition, ArcAndTokensVector& indicesOfCurrentPermutation) const{
	NonStrictMarking m(init_marking);
	for(ArcAndTokensVector::iterator iter = indicesOfCurrentPermutation.begin(); iter != indicesOfCurrentPermutation.end(); iter++){
		vector<unsigned int>& tokens = iter->modificationVector;

		for(vector< unsigned int >::const_iterator tokenIter = tokens.begin(); tokenIter < tokens.end(); tokenIter++){
			Token t((iter->enabledBy)[*tokenIter].getAge(), 1);
			iter->moveToken(t, m);
		}
	}

	for(OutputArc::WeakPtrVector::const_iterator postsetIter = transition.GetPostset().begin(); postsetIter != transition.GetPostset().end(); postsetIter++){
		Token t(0, postsetIter->lock()->GetWeight());
		m.AddTokenInPlace(postsetIter->lock()->OutputPlace(), t);
	}

	result.push_back(m);
}

void TimeDartSuccessorGenerator::PrintTransitionStatistics(std::ostream& out) const {
		out << std::endl << "TRANSITION STATISTICS";
		for (unsigned int i=0;i<numberoftransitions;i++) {
			if ((i) % 6 == 0) {
				out << std::endl;
				out << "<" << tapn.GetTransitions()[i]->GetName() << ":" << transitionStatistics[i] << ">";
			}
			else {
				out << " <"  <<tapn.GetTransitions()[i]->GetName() << ":" << transitionStatistics[i] << ">";
			}
		}
		out << std::endl;
		out << std::endl;
	}

} /* namespace DiscreteVerification */
} /* namespace VerifyTAPN */

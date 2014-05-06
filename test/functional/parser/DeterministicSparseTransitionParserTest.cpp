/*
 * DeterministicSparseTransitionParserTest.cpp
 *
 *  Created on: Feb 24, 2014
 *      Author: Manuel Sascha Weiand
 */

#include "gtest/gtest.h"
#include "storm-config.h"

#include "src/parser/DeterministicSparseTransitionParser.h"
#include "src/storage/SparseMatrix.h"
#include "src/settings/InternalOptionMemento.h"
#include "src/exceptions/FileIoException.h"
#include "src/exceptions/WrongFormatException.h"

TEST(DeterministicSparseTransitionParserTest, NonExistingFile) {

	// No matter what happens, please do NOT create a file with the name "nonExistingFile.not"!
	ASSERT_THROW(storm::parser::DeterministicSparseTransitionParser::parseDeterministicTransitions(STORM_CPP_TESTS_BASE_PATH "/nonExistingFile.not"), storm::exceptions::FileIoException);

	storm::storage::SparseMatrix<double> nullMatrix;
	ASSERT_THROW(storm::parser::DeterministicSparseTransitionParser::parseDeterministicTransitionRewards(STORM_CPP_TESTS_BASE_PATH "/nonExistingFile.not", nullMatrix), storm::exceptions::FileIoException);
}


TEST(DeterministicSparseTransitionParserTest, BasicTransitionsParsing) {

	// Parse a deterministic transitions file and test the resulting matrix.
	storm::storage::SparseMatrix<double> transitionMatrix = storm::parser::DeterministicSparseTransitionParser::parseDeterministicTransitions(STORM_CPP_TESTS_BASE_PATH "/functional/parser/tra_files/dtmc_general.tra");

	ASSERT_EQ(8, transitionMatrix.getColumnCount());
	ASSERT_EQ(21, transitionMatrix.getEntryCount());

	// Test every entry of the matrix.
	storm::storage::SparseMatrix<double>::const_iterator cIter = transitionMatrix.begin(0);

	ASSERT_EQ(0, cIter->getColumn());
	ASSERT_EQ(0, cIter->getValue());
	cIter++;
	ASSERT_EQ(1, cIter->getColumn());
	ASSERT_EQ(1, cIter->getValue());
	cIter++;
	ASSERT_EQ(1, cIter->getColumn());
	ASSERT_EQ(0, cIter->getValue());
	cIter++;
	ASSERT_EQ(2, cIter->getColumn());
	ASSERT_EQ(0.5, cIter->getValue());
	cIter++;
	ASSERT_EQ(3, cIter->getColumn());
	ASSERT_EQ(0.5, cIter->getValue());
	cIter++;
	ASSERT_EQ(2, cIter->getColumn());
	ASSERT_EQ(0, cIter->getValue());
	cIter++;
	ASSERT_EQ(3, cIter->getColumn());
	ASSERT_EQ(0.4, cIter->getValue());
	cIter++;
	ASSERT_EQ(4, cIter->getColumn());
	ASSERT_EQ(0.4, cIter->getValue());
	cIter++;
	ASSERT_EQ(5, cIter->getColumn());
	ASSERT_EQ(0.2, cIter->getValue());
	cIter++;
	ASSERT_EQ(3, cIter->getColumn());
	ASSERT_EQ(1, cIter->getValue());
	cIter++;
	ASSERT_EQ(3, cIter->getColumn());
	ASSERT_EQ(1, cIter->getValue());
	cIter++;
	ASSERT_EQ(4, cIter->getColumn());
	ASSERT_EQ(0, cIter->getValue());
	cIter++;
	ASSERT_EQ(3, cIter->getColumn());
	ASSERT_EQ(0.1, cIter->getValue());
	cIter++;
	ASSERT_EQ(4, cIter->getColumn());
	ASSERT_EQ(0.1, cIter->getValue());
	cIter++;
	ASSERT_EQ(5, cIter->getColumn());
	ASSERT_EQ(0.1, cIter->getValue());
	cIter++;
	ASSERT_EQ(6, cIter->getColumn());
	ASSERT_EQ(0.7, cIter->getValue());
	cIter++;
	ASSERT_EQ(0, cIter->getColumn());
	ASSERT_EQ(0.9, cIter->getValue());
	cIter++;
	ASSERT_EQ(5, cIter->getColumn());
	ASSERT_EQ(0, cIter->getValue());
	cIter++;
	ASSERT_EQ(6, cIter->getColumn());
	ASSERT_EQ(0.1, cIter->getValue());
	cIter++;
	ASSERT_EQ(6, cIter->getColumn());
	ASSERT_EQ(0.224653, cIter->getValue());
	cIter++;
	ASSERT_EQ(7, cIter->getColumn());
	ASSERT_EQ(0.775347, cIter->getValue());
}

TEST(DeterministicSparseTransitionParserTest, BasicTransitionsRewardsParsing) {

	// First parse a transition file. Then parse a transition reward file for the resulting transition matrix.
	storm::storage::SparseMatrix<double> transitionMatrix = storm::parser::DeterministicSparseTransitionParser::parseDeterministicTransitions(STORM_CPP_TESTS_BASE_PATH "/functional/parser/tra_files/dtmc_general.tra");

	storm::storage::SparseMatrix<double> rewardMatrix = storm::parser::DeterministicSparseTransitionParser::parseDeterministicTransitionRewards(STORM_CPP_TESTS_BASE_PATH "/functional/parser/rew_files/dtmc_general.trans.rew", transitionMatrix);

	ASSERT_EQ(8, rewardMatrix.getColumnCount());
	ASSERT_EQ(17, rewardMatrix.getEntryCount());

	// Test every entry of the matrix.
	storm::storage::SparseMatrix<double>::const_iterator cIter = rewardMatrix.begin(0);

	ASSERT_EQ(1, cIter->getColumn());
	ASSERT_EQ(10, cIter->getValue());
	cIter++;
	ASSERT_EQ(2, cIter->getColumn());
	ASSERT_EQ(5, cIter->getValue());
	cIter++;
	ASSERT_EQ(3, cIter->getColumn());
	ASSERT_EQ(5.5, cIter->getValue());
	cIter++;
	ASSERT_EQ(3, cIter->getColumn());
	ASSERT_EQ(21.4, cIter->getValue());
	cIter++;
	ASSERT_EQ(4, cIter->getColumn());
	ASSERT_EQ(4, cIter->getValue());
	cIter++;
	ASSERT_EQ(5, cIter->getColumn());
	ASSERT_EQ(2, cIter->getValue());
	cIter++;
	ASSERT_EQ(3, cIter->getColumn());
	ASSERT_EQ(1, cIter->getValue());
	cIter++;
	ASSERT_EQ(3, cIter->getColumn());
	ASSERT_EQ(1, cIter->getValue());
	cIter++;
	ASSERT_EQ(3, cIter->getColumn());
	ASSERT_EQ(0.1, cIter->getValue());
	cIter++;
	ASSERT_EQ(4, cIter->getColumn());
	ASSERT_EQ(1.1, cIter->getValue());
	cIter++;
	ASSERT_EQ(5, cIter->getColumn());
	ASSERT_EQ(9.5, cIter->getValue());
	cIter++;
	ASSERT_EQ(6, cIter->getColumn());
	ASSERT_EQ(6.7, cIter->getValue());
	cIter++;
	ASSERT_EQ(0, cIter->getColumn());
	ASSERT_EQ(1, cIter->getValue());
	cIter++;
	ASSERT_EQ(5, cIter->getColumn());
	ASSERT_EQ(0, cIter->getValue());
	cIter++;
	ASSERT_EQ(6, cIter->getColumn());
	ASSERT_EQ(12, cIter->getValue());
	cIter++;
	ASSERT_EQ(6, cIter->getColumn());
	ASSERT_EQ(35.224653, cIter->getValue());
	cIter++;
	ASSERT_EQ(7, cIter->getColumn());
	ASSERT_EQ(9.875347, cIter->getValue());
}


TEST(DeterministicSparseTransitionParserTest, Whitespaces) {

	// Test the resilience of the parser against whitespaces.
	// Do so by comparing the hash of the matrix resulting from the file without whitespaces with the hash of the matrix resulting from the file with whitespaces.
	uint_fast64_t correctHash = storm::parser::DeterministicSparseTransitionParser::parseDeterministicTransitions(STORM_CPP_TESTS_BASE_PATH "/functional/parser/tra_files/dtmc_general.tra").hash();
	storm::storage::SparseMatrix<double> transitionMatrix = storm::parser::DeterministicSparseTransitionParser::parseDeterministicTransitions(STORM_CPP_TESTS_BASE_PATH "/functional/parser/tra_files/dtmc_whitespaces.tra");
	ASSERT_EQ(correctHash, transitionMatrix.hash());

	// Do the same for the corresponding transition rewards file (with and without whitespaces)
	correctHash = storm::parser::DeterministicSparseTransitionParser::parseDeterministicTransitionRewards(STORM_CPP_TESTS_BASE_PATH "/functional/parser/rew_files/dtmc_general.trans.rew", transitionMatrix).hash();
	ASSERT_EQ(correctHash, storm::parser::DeterministicSparseTransitionParser::parseDeterministicTransitionRewards(STORM_CPP_TESTS_BASE_PATH "/functional/parser/rew_files/dtmc_whitespaces.trans.rew", transitionMatrix).hash());
}

TEST(DeterministicSparseTransitionParserTest, MixedTransitionOrder) {

	// Since the MatrixBuilder needs sequential input of new elements reordering of transitions or states should throw an exception.
	ASSERT_THROW(storm::parser::DeterministicSparseTransitionParser::parseDeterministicTransitions(STORM_CPP_TESTS_BASE_PATH "/functional/parser/tra_files/dtmc_mixedTransitionOrder.tra"), storm::exceptions::InvalidArgumentException);
	ASSERT_THROW(storm::parser::DeterministicSparseTransitionParser::parseDeterministicTransitions(STORM_CPP_TESTS_BASE_PATH "/functional/parser/tra_files/dtmc_mixedStateOrder.tra"), storm::exceptions::InvalidArgumentException);

	storm::storage::SparseMatrix<double> transitionMatrix = storm::parser::DeterministicSparseTransitionParser::parseDeterministicTransitions(STORM_CPP_TESTS_BASE_PATH "/functional/parser/tra_files/dtmc_general.tra");
	ASSERT_THROW(storm::parser::DeterministicSparseTransitionParser::parseDeterministicTransitionRewards(STORM_CPP_TESTS_BASE_PATH "/functional/parser/rew_files/dtmc_mixedTransitionOrder.trans.rew", transitionMatrix), storm::exceptions::InvalidArgumentException);
	ASSERT_THROW(storm::parser::DeterministicSparseTransitionParser::parseDeterministicTransitionRewards(STORM_CPP_TESTS_BASE_PATH "/functional/parser/rew_files/dtmc_mixedStateOrder.trans.rew", transitionMatrix), storm::exceptions::InvalidArgumentException);
}

TEST(DeterministicSparseTransitionParserTest, FixDeadlocks) {

	// Set the fixDeadlocks flag temporarily. It is set to its old value once the deadlockOption object is destructed.
	storm::settings::InternalOptionMemento setDeadlockOption("fixDeadlocks", true);

	// Parse a transitions file with the fixDeadlocks Flag set and test if it works.
	storm::storage::SparseMatrix<double> transitionMatrix = storm::parser::DeterministicSparseTransitionParser::parseDeterministicTransitions(STORM_CPP_TESTS_BASE_PATH "/functional/parser/tra_files/dtmc_deadlock.tra");

	ASSERT_EQ(9, transitionMatrix.getColumnCount());
	ASSERT_EQ(23, transitionMatrix.getEntryCount());

	storm::storage::SparseMatrix<double>::const_iterator cIter = transitionMatrix.begin(7);
	ASSERT_EQ(7, cIter->getColumn());
	ASSERT_EQ(1, cIter->getValue());
	cIter++;
	ASSERT_EQ(6, cIter->getColumn());
	ASSERT_EQ(0.224653, cIter->getValue());
	cIter++;
	ASSERT_EQ(7, cIter->getColumn());
	ASSERT_EQ(0.775347, cIter->getValue());
	cIter++;
	ASSERT_EQ(8, cIter->getColumn());
	ASSERT_EQ(0, cIter->getValue());
}

TEST(DeterministicSparseTransitionParserTest, DontFixDeadlocks) {

	// Try to parse a transitions file containing a deadlock state with the fixDeadlocksFlag unset. This should throw an exception.
	storm::settings::InternalOptionMemento unsetDeadlockOption("fixDeadlocks", false);

	ASSERT_THROW(storm::parser::DeterministicSparseTransitionParser::parseDeterministicTransitions(STORM_CPP_TESTS_BASE_PATH "/functional/parser/tra_files/dtmc_deadlock.tra"), storm::exceptions::WrongFormatException);
}

TEST(DeterministicSparseTransitionParserTest, DoubledLines) {

	// There is a redundant line in the transition file. As the transition already exists this should throw an exception.
	// Note: If two consecutive lines are doubled no exception is thrown.
	ASSERT_THROW(storm::parser::DeterministicSparseTransitionParser::parseDeterministicTransitions(STORM_CPP_TESTS_BASE_PATH "/functional/parser/tra_files/dtmc_doubledLines.tra"), storm::exceptions::InvalidArgumentException);
}

TEST(DeterministicSparseTransitionParserTest, RewardForNonExistentTransition) {

	// First parse a transition file. Then parse a transition reward file for the resulting transition matrix.
	storm::storage::SparseMatrix<double> transitionMatrix = storm::parser::DeterministicSparseTransitionParser::parseDeterministicTransitions(STORM_CPP_TESTS_BASE_PATH "/functional/parser/tra_files/dtmc_general.tra");

	// There is a reward for a transition that does not exist in the transition matrix.
	ASSERT_THROW(storm::parser::DeterministicSparseTransitionParser::parseDeterministicTransitionRewards(STORM_CPP_TESTS_BASE_PATH "/functional/parser/rew_files/dtmc_rewardForNonExTrans.trans.rew", transitionMatrix), storm::exceptions::WrongFormatException);
}

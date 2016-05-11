//------------------------------------------------------------------------------
//  GBM by Greg Ridgeway  Copyright (C) 2003
//
//  File:       node_search.cpp
//
//------------------------------------------------------------------------------
//-----------------------------------
// Includes
//-----------------------------------
#include "node_search.h"

//----------------------------------------
// Function Members - Public
//----------------------------------------
CNodeSearch::CNodeSearch(int numColData, unsigned long minObs):
variableSplitters(numColData, VarSplitter(minObs))
{
    cTerminalNodes = 1;
    minNumObs = minObs;

}


CNodeSearch::~CNodeSearch()
{
}


void CNodeSearch::Reset()
{
	cTerminalNodes = 1;
}

void CNodeSearch::GenerateAllSplits
(
		vector<CNode*>& vecpTermNodes,
		const CDataset& data,
		double* adZ,
		vector<unsigned long>& aiNodeAssign
)
{
	unsigned long iWhichObs = 0;
	const CDataset::index_vector colNumbers(data.random_order());
	const CDataset::index_vector::const_iterator final = colNumbers.begin() + data.get_numFeatures();

	// Loop over terminal nodes
	for(long iNode = 0; iNode < cTerminalNodes; iNode++)
	{
	  // If node already has a split assigned then skip
	  if(vecpTermNodes[iNode]->splitAssigned) continue;

	  // Reset variable splitters
	  ResetVarSplitter();

	  // Loop over variables - Generate splits
	  for(CDataset::index_vector::const_iterator it=colNumbers.begin();
			  it != final;
			  it++)
	  {
		  variableSplitters[*it].SetForNode(*vecpTermNodes[iNode]);
		  variableSplitters[*it].SetForVariable(*it, data.varclass(*it));
		  for(long iOrderObs=0; iOrderObs < data.get_trainSize(); iOrderObs++)
		  {
			  //Get Observation and add to split if needed
			  iWhichObs = data.order_ptr()[(*it)*data.get_trainSize() + iOrderObs];
			  if((aiNodeAssign[iWhichObs] == iNode) && data.GetBag()[iWhichObs])
			  {
				  const double dX = data.x_value(iWhichObs, *it);
				  variableSplitters[*it].IncorporateObs(dX,
									adZ[iWhichObs],
									data.weight_ptr()[iWhichObs],
									data.monotone(*it));
			  }

		  }
		  if(data.varclass(*it) != 0) // evaluate if categorical split
		  {
			  variableSplitters[*it].EvaluateCategoricalSplit();
		  }

	  }
	  // Assign best split to node
	  AssignToNode(*vecpTermNodes[iNode]);
	}
}


double CNodeSearch::SplitAndCalcImprovement
(
		vector<CNode*>& vecpTermNodes,
		const CDataset& data,
		vector<unsigned long>& aiNodeAssign
)
{
	// search for the best split
	long iBestNode = 0;
	double dBestNodeImprovement = 0.0;
	for(long iNode=0; iNode < cTerminalNodes; iNode++)
	{
		if(vecpTermNodes[iNode]->SplitImprovement() > dBestNodeImprovement)
		{
			iBestNode = iNode;
			dBestNodeImprovement = vecpTermNodes[iNode]->SplitImprovement();
		}
	}
	// Split Node if improvement is non-zero
	if(dBestNodeImprovement != 0.0)
	{
		//Split Node
		vecpTermNodes[iBestNode]->SplitNode();
		cTerminalNodes += 2;

		// Move data to children nodes
		ReAssignData(iBestNode, vecpTermNodes, data, aiNodeAssign);

		// Add children to terminal node list
		vecpTermNodes[cTerminalNodes-2] = vecpTermNodes[iBestNode]->pRightNode;
		vecpTermNodes[cTerminalNodes-1] = vecpTermNodes[iBestNode]->pMissingNode;
		vecpTermNodes[iBestNode] = vecpTermNodes[iBestNode]->pLeftNode;
	}

	return dBestNodeImprovement;
}

//----------------------------------------
// Function Members - Private
//----------------------------------------
void CNodeSearch::ReAssignData
(
		long splittedNodeIndex,
		vector<CNode*>& vecpTermNodes,
		const CDataset& data,
		vector<unsigned long>& aiNodeAssign
)
{
	// assign observations to the correct node
	for(long iObs=0; iObs < data.get_trainSize(); iObs++)
	{
		if(aiNodeAssign[iObs]==splittedNodeIndex)
		{
		  signed char schWhichNode = vecpTermNodes[splittedNodeIndex]->WhichNode(data,iObs);
		  if(schWhichNode == 1) // goes right
		  {
			  aiNodeAssign[iObs] = cTerminalNodes-2;
		  }
		  else if(schWhichNode == 0) // is missing
		  {
			  aiNodeAssign[iObs] = cTerminalNodes-1;
		  }
		  // those to the left stay with the same node assignment
		  }
	}
}

void CNodeSearch::AssignToNode(CNode& terminalNode)
{
	// Find the best split
	long bestSplitInd = 0;
	double bestErrImprovement = 0.0;
	double currErrImprovement = 0.0;

	for(long it = 0; it < variableSplitters.size(); it++)
	{
		currErrImprovement = variableSplitters[it].GetBestImprovement();
		if(currErrImprovement > bestErrImprovement)
		{
			bestErrImprovement = currErrImprovement;
			bestSplitInd = it;
		}
	}

	// Check if variables explored have better improvement than previous searches
	if(bestErrImprovement > terminalNode.childrenParams.GetImprovement())
	{
		terminalNode.childrenParams =  variableSplitters[bestSplitInd].GetBestSplit();
	}

	// Set the nodes flag to indicate it is now split
	terminalNode.SplitAssign();
}


void CNodeSearch::ResetVarSplitter()
{
	// Reset each variable splitter
	for(long it = 0; it < variableSplitters.size(); it++)
	{
		variableSplitters[it].Reset();
	}

}

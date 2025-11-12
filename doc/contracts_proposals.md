# Proposal voting

Proposal voting is the the on-chain way of decision-making.
It is implemented in smart contracts with support of the QPI.

There are some general characteristics of the proposal voting:

- Proposal voting is implemented in smart contracts.
- A new proposal is open for voting until the end of the epoch. After the epoch transition, it changes its state from active to finished.
- Each proposal has a type and some types of proposals commonly trigger action (such as setting a contract state variable or transferring QUs to another entity) after the end of the epoch if the proposal is accepted by getting enough votes.
- The proposer entity can have at most one proposal at a time. Setting a new proposal with the same seed will overwrite the previous one.
- Number of simultaneous proposals per epoch is limited as configured by the contract. The data structures storing the proposal and voting state are stored as a part of the contract state.
- In this data storage, commonly named `state.proposals`, and the function/procedure interface, each proposal is identified by a proposal index.
- The types of proposals that are allowed are restricted as configured by the contract.
- The common types of proposals have a predefined set of options that the voters can vote for. Option 0 is always "no change".
- Each vote, which is connected to each voter entity, can have a value (most commonly an option index) or `NO_VOTE_VALUE` (which means abstaining).
- The entities that are allowed to create/change/cancel proposals are configured by the contract dev. The same applies to the entities that are allowed to vote. Checking eligibility is done by the proposal QPI functions provided.
- The common rule for accepting a proposal option (the one with most votes) is that at least 2/3 of the eligible votes have been casted and that at least the option gets more than 1/3 of the eligible votes.
- Depending on the required features, different data structures can be used. The most lightweight option (in terms of storage, compute, and network bandwidth) is supporting yes/no proposals only (2 options).

In the following, we first address the application that is most relevant for contract devs, which is shareholder voting about state variables, such as fees.

1. [Introduction to Shareholder Proposals](#introduction-to-shareholder-proposals)
2. [Understanding Shareholder Proposal Voting Implementation](#understanding-shareholder-proposal-voting-implementation)
3. [Voting by Computors](#voting-by-computors)
4. [Proposal Types](#proposal-types)


## Introduction to Shareholder Proposals

The entities possessing the 676 shares of the contract (initially sold in the IPO) are the contract shareholders.
Typically, they get dividends from the contract revenues and decide about contract-related topics via voting.

The most relevant use case of voting is changing state variables that control the behavior of the contract. For example:

- how many QUs need to be paid as fees for using the contract procedures,
- how much of the revenue is paid as dividends to the shareholders,
- how much of the revenue is burned.

Next to the general characteristics described in the section above, shareholder voting has the following features:

- Only contract shareholders can propose and vote.
- There is one vote per share, so one shareholder can have multiple votes.
- A shareholder can distribute their votes to multiple vote values.
- Shares can be sold during the epoch. The right to vote in an active proposal isn't sold with the share, but the right to vote is fixed to the entities possessing the share in the moment of creating/changing the proposal.
- Contracts can be shareholders of other contracts. For that case, there are special requirements to facilitate voting and creating proposals discussed [here](#contracts-as-shareholders-of-other-contracts).
- Standardized interface to reuse existing tools for shareholder voting in new contracts.
- Macros for implementing the most common use cases as easily as possible.


### The easiest way to support shareholder voting

The following shows how to easily implement the default shareholder proposal voting in your contract.

Features:

- Yes/no proposals for changing a single state variable per proposal,
- Usual checking that the invocator has the right to propose / vote and that the proposal data is valid,
- Check that proposed variable values are not negative (default use case: invocation fees),
- Check that index of variable in proposal is less than number of variables configured by contract dev,
- Allow charging fee for creating/changing/canceling proposal in order to avoid spam (fee is burned),
- Compatible with shareholder proposal voting interface already implemented in qubic-cli

If you need more than these features, go through the following steps anyway and continue reading the section about understanding the shareholder voting implementation.

#### 1. Setup proposal storage

First, you need to add the proposal storage to your contract state.
You can easily do this using the QPI macro `DEFINE_SHAREHOLDER_PROPOSAL_STORAGE(numProposalSlots, assetName)`.
With the yes/no shareholder proposals supported by this macro, each proposal slot occupies 22144 Bytes of state memory.
The number of proposal slots limits how many proposals can be open for voting simultaneously.
The `assetName` that you have to pass as the second argument is the `uint64` representation of your contract's 7-character asset name.

You can get the `assetName` by running the following code in any contract test code (such as "test/contract_qutil.cpp"):

```C++
std::cout << assetNameFromString("QUTIL") << std::endl;
```

Replace "QUTIL" by your contract's asset name as given in `contractDescriptions` in `src/contact_core/contract_def.h`.
You will get an integer that we recommend to assign to a `constexpr uint64` with a name following the scheme `QUTIL_CONTRACT_ASSET_NAME`.

When you have decided about the number of proposal slots and found out the the asset name, you can define the proposal storage similarly to this example taken from the contract QUTIL:

```C++
struct QUTIL
{
    // other state variables ...

    DEFINE_SHAREHOLDER_PROPOSAL_STORAGE(8, QUTIL_CONTRACT_ASSET_NAME);

    // ...
};
```

`DEFINE_SHAREHOLDER_PROPOSAL_STORAGE` defines a state object `state.proposals` and the types `ProposalDataT`, `ProposersAndVotersT`, and `ProposalVotingT`.
Make sure to have no name clashes with these.
Using other names isn't possible if you want to benefit from the QPI macros for simplifying the implementation.


#### 2. Implement code for updating state variables

After voting in a proposal is closed, the results need to be checked and state variables may need to be updated.
This typically happens in the system procedure `END_EPOCH`.

In order to simplify the implementation of the default case, we provide a QPI macro for implementing a procedure `FinalizeShareholderStateVarProposals()` that you can call from `END_EPOCH`.

The macro `IMPLEMENT_FinalizeShareholderStateVarProposals()` can be used as follow:

```C++
struct QUTIL
{
    // ...

    IMPLEMENT_FinalizeShareholderStateVarProposals()
    {
        // When you call FinalizeShareholderStateVarProposals(), the following code is run for each
        // proposal of the current epoch that has been accepted.
        //
        // Your code should set the state variable that the proposal is about to the accepted value.
        // This can be done as in this example taken from QUTIL:

        switch (input.proposal.variableOptions.variable)
        {
        case 0:
            state.smt1InvocationFee = input.acceptedValue;
            break;
        case 1:
            state.pollCreationFee = input.acceptedValue;
            break;
        case 2:
            state.pollVoteFee = input.acceptedValue;
            break;
        case 3:
            state.distributeQuToShareholderFeePerShareholder = input.acceptedValue;
            break;
        case 4:
            state.shareholderProposalFee = input.acceptedValue;
            break;
        }
    }

    // ...
}
```

The next step is to call `FinalizeShareholderStateVarProposals()` in `END_EPOCH`, in order to make sure the state variables are changed after a proposal has been accepted.

```C++
    END_EPOCH()
    {
        // ...
        CALL(FinalizeShareholderStateVarProposals, input, output);
        // ...
    }
```

Note that `input` and `output` are instances of `NoData` passed to `END_EPOCH` (due to the requirements of the generalized contract procedure/function interface).
`FinalizeShareholderStateVarProposals` also has `NoData` as input and output, so the variables `input` and `output` available in `END_EPOCH` can be used instead of adding empty locals.


#### 3. Add default implementation of required procedures and functions

The required procedures and functions of the standardized shareholder proposal voting interface can be implemented with the macro:
`IMPLEMENT_DEFAULT_SHAREHOLDER_PROPOSAL_VOTING(numFeeStateVariables, setProposalFeeVarOrValue)`.

As `numFeeStateVariables`, pass the number of non-negative state variables (5 in the QUTIL example).

`setProposalFeeVarOrValue` is the fee to be payed for adding/changing/canceling a shareholder proposal. Here you may pass as a state variable or a constant value.

The macro can be used as follows (example from QUTIL):

```C++
struct QUTIL
{
    // ...

    IMPLEMENT_DEFAULT_SHAREHOLDER_PROPOSAL_VOTING(5, state.shareholderProposalFee)

    // ...
}
```

#### 4. Register required procedures and functions

Finally, the required procedures and functions of the standardized shareholder proposal voting interface can be registered with the macro
`REGISTER_SHAREHOLDER_PROPOSAL_VOTING()` in `REGISTER_USER_FUNCTIONS_AND_PROCEDURES()`. Example:

```C++
    REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
    {
        // ...

        REGISTER_SHAREHOLDER_PROPOSAL_VOTING();
    }
```

Please note that `REGISTER_SHAREHOLDER_PROPOSAL_VOTING()` registers the following input types:

- user functions 65531 - 65535 via `REGISTER_USER_FUNCTION`
- user procedures 65534 - 65535 via `REGISTER_USER_PROCEDURE`

So make sure that your contract does not use these input types for other functions or procedures.

#### 5. Test with qubic-cli

After following the steps above, your contract is ready for changing fees and similar state variables via shareholder proposal voting.
When you run the node with your contract, you may test the voting with [qubic-cli](https://github.com/qubic/qubic-cli).

In order to enable it, you need to add your contract to the array `shareholderProposalSupportPerContract` in `proposals.cpp` (with the type `DefaultYesNoSingleVarShareholderProposalSupported`).
Now you should be able to use `-setshareholderproposal` and the other commands with your contract.

Optionally, if you want to use your contract name as ContractIndex when running qubic-cli, you may add it to `getContractIndex()` in `argparser.h`.


## Understanding Shareholder Proposal Voting Implementation

In the section above, several macros have been used to easily implement a shareholder voting system that should suit the needs of many contracts.
If you require additional features, continue reading in order to learn how to get them.

### Required procedures, functions, types, and data

The following elements are required to support shareholder proposals and voting:

- `END_EPOCH`: This system procedure is required in order to change your state variables if a proposal is accepted. The macro `IMPLEMENT_FinalizeShareholderStateVarProposals` provides a convenient implementation for simple use cases that can be called from `END_EPOCH`.
- `SetShareholderProposal`: This procedure is for creating, changing, and canceling shareholder proposals. It often requires a custom implementation, because it checks custom requirements defined by the contract developers, for example about which types of proposals are allowed and if the proposed values are valid for the contract.
- `GetShareholderProposal`: This function returns the proposal data (without votes or summarized results). This only requires a custom implementation if you want to support changing multiple variables in one proposal.
- `GetShareholderProposalIndices`: This function lists proposal indices of active or finished proposals. It usually doesn't require a custom implementation.
- `GetShareholderProposalFees`: This function returns the fee that is to be paid for invoking `SetShareholderProposal` and `SetShareholderVotes`. If you want to charge a fee `SetShareholderVotes` (default is no fee), you need a custom implementation of both `GetShareholderProposalFees` and `SetShareholderVotes`.
- `SetShareholderVotes`: Procedure for setting the votes. Only requires a custom implementation if your want to charge fees.
- `GetShareholderVotes`: Function for getting the votes of a shareholder. Usually shouldn't require a custom implementation.
- `GetShareholderVotingResults`: Function for getting the vote results summary. Usually doesn't require a custom implementation.
- `SET_SHAREHOLDER_PROPOSAL` and `SET_SHAREHOLDER_VOTES`: These are notification procedures required to handle voting of other contracts that are shareholder of your contract. They usually just invoke `SetShareholderProposal` or `SetShareholderVote`, respectively.
- Proposal data storage and types: The default implementations expect the object `state.proposals` and the types `ProposalDataT`, `ProposersAndVotersT`, and `ProposalVotingT`, which can be defined via `DEFINE_SHAREHOLDER_PROPOSAL_STORAGE` in some cases.

QPI provides default implementations through several macros, as used in the [Introduction to Shareholder Proposals](#introduction-to-shareholder-proposals).
The following tables gives an overview about when the macros can be used.
In the columns there are four use cases, three with one variable per proposal suitable if the variables are independent of each other.
Yes/no is the default addressed with the simple macro implementation above.
N option is if you want to support proposals more than 2 options (yes/no), like: value1, value2, value3, or "no change"?
Scalar means that every vote can have a different scalar value. The current implementation computes the mean value of all votes as the final voting result.
Finally, multi-variable proposals change more than one variable if accepted. These make sense if the variables in the state cannot be changed independently of each other, but must be set together in order to ensure keeping a valid state.


Default implementation can be used?             | 1-var yes/no | 1-var N option | 1-var scalar | multi-var
------------------------------------------------|--------------|----------------|--------------|-----------
`DEFINE_SHAREHOLDER_PROPOSAL_STORAGE`           | X            |                |              | X
`IMPLEMENT_FinalizeShareholderStateVarProposals`| X            |                |              |
`IMPLEMENT_SetShareholderProposal`              | X            |                |              |
`IMPLEMENT_GetShareholderProposal`              | X            | X              | X            |
`IMPLEMENT_GetShareholderProposalIndices`       | X            | X              | X            | X
`IMPLEMENT_GetShareholderProposalFees`          | X            | X              | X            | X
`IMPLEMENT_SetShareholderVotes`                 | X            | X              | X            | X
`IMPLEMENT_GetShareholderVotes`                 | X            | X              | X            | X
`IMPLEMENT_GetShareholderVotingResults`         | X            | X              | X            | X
`IMPLEMENT_SET_SHAREHOLDER_PROPOSAL`            | X            | X              | X            | X
`IMPLEMENT_SET_SHAREHOLDER_VOTES`               | X            | X              | X            | X
tool `qubic-cli -shareholder*`                  | X            | X              | soon         |
Example contracts                               | QUTIL        | TESTEXB        | TESTEXB      | TESTEXA


If you need a custom implementation of one of the elements, I recommend to start with the default implementations given below.
You may also have a look into the example contracts given in the table.


#### Proposal types and storage

The default implementation of `DEFINE_SHAREHOLDER_PROPOSAL_STORAGE(assetNameInt64, numProposalSlots)` is defined as follows:

```C++
    public:
        typedef ProposalDataYesNo ProposalDataT;
        typedef ProposalAndVotingByShareholders<numProposalSlots, assetNameInt64> ProposersAndVotersT;
        typedef ProposalVoting<ProposersAndVotersT, ProposalDataT> ProposalVotingT;
    protected:
        ProposalVotingT proposals;
```

With `ProposalDataT` your have the following options:
- `ProposalDataYesNo`: Proposals with two options, yes (change) and no (no change)
- `ProposalDataV1<false>`: Support multi-option proposals with up to 5 options for the Variable and Transfer proposal types and up to 8 options for GeneralProposal and MultiVariables. No scalar voting support.
- `ProposalDataV1<true>`: Support scalar voting and multi-option voting. This leads to the highest resource requirements, because 8 bytes of storage are required per proposal and voter.

`ProposersAndVotersT` defines the class used to manage rights to propose and vote. It's important that you pass the correct asset name of your contract. Otherwise it won't find the right shareholders.
The number of proposal slots linearly scales the storage and digest compute requirements. So we recommend to use a quite low number here, similar to the number of variables that can be set in your state.

`ProposalVotingT` combines `ProposersAndVotersT` and `ProposalDataT` into the class used for storing all proposal and voting data.
It is instantiated as `state.proposals`.

In order to support MultiVariables proposals that allow to change multiple variables in a single proposal, the variable values need to be stored separately, for example in an array of `numProposalSlots` structs, one for each potential proposal.
See the contract TestExampleA to see how to support multi-variable proposals.


#### User Procedure FinalizeShareholderStateVarProposals for easy implementation of END_EPOCH

This default implementation works for yes/no proposals. For a version that supports more options and scalar votes, see the contract "TestExampleB".


```C++
IMPLEMENT_FinalizeShareholderStateVarProposals()
{
    // your code for setting state variable, which is called by FinalizeShareholderStateVarProposals()
    // after a proposal has been accepted
}
```

The implementation above is expanded to the following code:

```C++
struct FinalizeShareholderProposalSetStateVar_input
{
    sint32 proposalIndex;
    ProposalDataT proposal;
    ProposalSummarizedVotingDataV1 results;
    sint32 acceptedOption;
    sint64 acceptedValue;
};
typedef NoData FinalizeShareholderProposalSetStateVar_output;

typedef NoData FinalizeShareholderStateVarProposals_input;
typedef NoData FinalizeShareholderStateVarProposals_output;
struct FinalizeShareholderStateVarProposals_locals
{
    FinalizeShareholderProposalSetStateVar_input p;
    uint16 proposalClass;
};

PRIVATE_PROCEDURE_WITH_LOCALS(FinalizeShareholderStateVarProposals)
{
    // Analyze proposal results and set variables:
    // Iterate all proposals that were open for voting in this epoch ...
    locals.p.proposalIndex = -1;
    while ((locals.p.proposalIndex = qpi(state.proposals).nextProposalIndex(locals.p.proposalIndex, qpi.epoch())) >= 0)
    {
        if (!qpi(state.proposals).getProposal(locals.p.proposalIndex, locals.p.proposal))
            continue;

        locals.proposalClass = ProposalTypes::cls(locals.p.proposal.type);

        // Handle proposal type Variable / MultiVariables
        if (locals.proposalClass == ProposalTypes::Class::Variable || locals.proposalClass == ProposalTypes::Class::MultiVariables)
        {
            // Get voting results and check if conditions for proposal acceptance are met
            if (!qpi(state.proposals).getVotingSummary(locals.p.proposalIndex, locals.p.results))
                continue;

            locals.p.acceptedOption = locals.p.results.getAcceptedOption();
            if (locals.p.acceptedOption <= 0)
                continue;

            locals.p.acceptedValue = locals.p.proposal.variableOptions.value;

            CALL(FinalizeShareholderProposalSetStateVar, locals.p, output);
        }
    }
}

PRIVATE_PROCEDURE(FinalizeShareholderProposalSetStateVar)
{
    // your code for setting state variable, which is called by FinalizeShareholderStateVarProposals()
    // after a proposal has been accepted
}
```


#### User Procedure SetShareholderProposal

The default implementation provided by `IMPLEMENT_SetShareholderProposal(numFeeStateVariables, setProposalFeeVarOrValue)` supports yes/no for setting one variable per proposal, like "I propose to change transfer fee to 1000 QU. Yes or no?".
Although it only supports one variable per proposal, an arbitrary number of different variables can be supported across multiple proposals.

```C++
typedef ProposalDataT SetShareholderProposal_input;
typedef uint16 SetShareholderProposal_output;

PUBLIC_PROCEDURE(SetShareholderProposal)
{
    // check proposal input and fees
    if (qpi.invocationReward() < setProposalFeeVarOrValue || (input.epoch
        && (input.type != ProposalTypes::VariableYesNo || input.variableOptions.variable >= numFeeStateVariables
            || input.variableOptions.value < 0)))
    {
        // error -> reimburse invocation reward
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        output = INVALID_PROPOSAL_INDEX;
        return;
    }

    // try to set proposal (checks invocator's rights and general validity of input proposal), returns proposal index
    output = qpi(state.proposals).setProposal(qpi.invocator(), input);
    if (output == INVALID_PROPOSAL_INDEX)
    {
        // error -> reimburse invocation reward
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        return;
    }

    // burn fee and reimburse if payed too much
    qpi.burn(setProposalFeeVarOrValue);
    if (qpi.invocationReward() > setProposalFeeVarOrValue)
        qpi.transfer(qpi.invocator(), qpi.invocationReward() - setProposalFeeVarOrValue);
}
```

Note that `input.epoch == 0` means clearing a proposal.
Returning the invalid proposal `output = INVALID_PROPOSAL_INDEX` signals an error.


#### User Function GetShareholderProposal

`IMPLEMENT_GetShareholderProposal()` defines the following user function, which returns the proposal without votes or summarized results:

```C++
struct GetShareholderProposal_input
{
    uint16 proposalIndex;
};
struct GetShareholderProposal_output
{
    ProposalDataT proposal;
    id proposerPubicKey;
};

PUBLIC_FUNCTION(GetShareholderProposal)
{
    // On error, output.proposal.type is set to 0
    output.proposerPubicKey = qpi(state.proposals).proposerId(input.proposalIndex);
    qpi(state.proposals).getProposal(input.proposalIndex, output.proposal);
}
```


#### User Function GetShareholderProposalIndices

The macro `IMPLEMENT_GetShareholderProposalIndices()` adds the following contract user function, which is required for listing
the active and inactive proposals.

```C++
struct GetShareholderProposalIndices_input
{
    bit activeProposals;		// Set true to return indices of active proposals, false for proposals of prior epochs
    sint32 prevProposalIndex;   // Set -1 to start getting indices. If returned index array is full, call again with highest index returned.
};
struct GetShareholderProposalIndices_output
{
    uint16 numOfIndices;		// Number of valid entries in indices. Call again if it is 64.
    Array<uint16, 64> indices;	// Requested proposal indices. Valid entries are in range 0 ... (numOfIndices - 1).
};

PUBLIC_FUNCTION(GetShareholderProposalIndices)
{
    if (input.activeProposals)
    {
        // Return proposals that are open for voting in current epoch
        // (output is initialized with zeros by contract system)
        while ((input.prevProposalIndex = qpi(state.proposals).nextProposalIndex(input.prevProposalIndex, qpi.epoch())) >= 0)
        {
            output.indices.set(output.numOfIndices, input.prevProposalIndex);
            ++output.numOfIndices;

            if (output.numOfIndices == output.indices.capacity())
                break;
        }
    }
    else
    {
        // Return proposals of previous epochs not overwritten yet
        // (output is initialized with zeros by contract system)
        while ((input.prevProposalIndex = qpi(state.proposals).nextFinishedProposalIndex(input.prevProposalIndex)) >= 0)
        {
            output.indices.set(output.numOfIndices, input.prevProposalIndex);
            ++output.numOfIndices;

            if (output.numOfIndices == output.indices.capacity())
                break;
        }
    }
}
```


#### User Function GetShareholderProposalFees

`IMPLEMENT_GetShareholderProposalFees(setProposalFeeVarOrValue)` provides the following implementation for returning fees for invoking the shareholder proposal and voting procedures.

```C++
typedef NoData GetShareholderProposalFees_input;
struct GetShareholderProposalFees_output
{
    sint64 setProposalFee;
    sint64 setVoteFee;
};

PUBLIC_FUNCTION(GetShareholderProposalFees)
{
    output.setProposalFee = setProposalFeeVarOrValue;
    output.setVoteFee = 0;
}
```

The default implementation assumes no vote fee, but the output struct and qubic-cli support voting fees.
So if you want to charge a fee for invoking `SetShareholderVotes`, you can copy the template above and just replace the 0.


#### User Procedure SetShareholderVotes

The default implementation of `IMPLEMENT_SetShareholderVotes()` supports all use cases, but does not charge a fee:

```C++
typedef ProposalMultiVoteDataV1 SetShareholderVotes_input;
typedef bit SetShareholderVotes_output;

PUBLIC_PROCEDURE(SetShareholderVotes)
{
    output = qpi(state.proposals).vote(qpi.invocator(), input);
}
```

#### User Function GetShareholderVotes

`IMPLEMENT_GetShareholderVotes()` provides the default implementation for returning the votes of a specific voter / shareholder.

```C++
struct GetShareholderVotes_input
{
    id voter;
    uint16 proposalIndex;
};
typedef ProposalMultiVoteDataV1 GetShareholderVotes_output;

PUBLIC_FUNCTION(GetShareholderVotes)
{
    // On error, output.votes.proposalType is set to 0
    qpi(state.proposals).getVotes(input.proposalIndex, input.voter, output);
}
```

#### User Function GetShareholderVotingResults

`IMPLEMENT_GetShareholderVotingResults()` provides the function returning the overall voting results of a proposal:

```C++
struct GetShareholderVotingResults_input
{
    uint16 proposalIndex;
};
typedef ProposalSummarizedVotingDataV1 GetShareholderVotingResults_output;

PUBLIC_FUNCTION(GetShareholderVotingResults)
{
    // On error, output.totalVotesAuthorized is set to 0
    qpi(state.proposals).getVotingSummary(
        input.proposalIndex, output);
}
```

### Contracts as shareholders of other contracts

When a contract A is shareholder of another contract B, the user procedures `B::SetShareholderProposal()` and `B::SetShareholderVotes()` cannot invoked directly via transaction as usual for "normal" user entities.

Further, the contract owning the share may be older than the contract it is shareholder of.
So the procedures cannot be invoked as in other contract interaction, because contracts can only access procedure/function of older contracts.

In order to provide a solution for this issue, the two QPI calls `qpi.setShareholderProposal()` and `qpi.setShareholderVotes()` were introduced.
They can be called from contract A and invoke the system procedures `B::SET_SHAREHOLDER_PROPOSAL` and `B::SET_SHAREHOLDER_VOTES`, respectively.
The system procedure `B::SET_SHAREHOLDER_PROPOSAL` and `B::SET_SHAREHOLDER_VOTES` call the user procedures `B::SetShareholderProposal()` and `B::SetShareholderVotes()`.

The mechanism is shown with both contracts TestExampleA and TestExampleB, which are shareholder of each other in `test/contract_testex.cpp`.
A custom user procedure `SetProposalInOtherContractAsShareholder` calls `qpi.setShareholderProposal()` with input given by the invocator who must be an administrator (or someone else who is allowed to create proposals in the name of the contract).
Similarly, a custom user procedure `SetVotesInOtherContractAsShareholder` calls `qpi.setShareholderVotes()` with input given by the invocator who must be an administrator (or someone else who is allowed to cast votes in the name of the contract, for example based on a community poll).


#### System Procedure SET_SHAREHOLDER_PROPOSAL

`IMPLEMENT_SET_SHAREHOLDER_PROPOSAL()` adds a system procedure invoked when `qpi.setShareholderProposal()` is called in another contract that is shareholder of your contract and wants to create/change/cancel a shareholder proposal in your contract.
The input is a generic buffer of 1024 bytes size that is copied into the input structure of `SetShareholderProposal` before calling this procedure.
`SetShareholderProposal_input` may be custom defined, as in multi-variable proposals. That is why a generic buffer is needed in the cross-contract interaction.

```C++
struct SET_SHAREHOLDER_PROPOSAL_locals
{
    SetShareholderProposal_input userProcInput;
};

SET_SHAREHOLDER_PROPOSAL_WITH_LOCALS()
{
    copyFromBuffer(locals.userProcInput, input);
    CALL(SetShareholderProposal, locals.userProcInput, output);
}
```

The input and output of `SET_SHAREHOLDER_PROPOSAL` are defined as follows in `qpi.h`:

```C++
	// Input of SET_SHAREHOLDER_PROPOSAL system procedure (buffer for passing the contract-dependent proposal data)
	typedef Array<uint8, 1024> SET_SHAREHOLDER_PROPOSAL_input;

	// Output of SET_SHAREHOLDER_PROPOSAL system procedure (proposal index, or INVALID_PROPOSAL_INDEX on error)
	typedef uint16 SET_SHAREHOLDER_PROPOSAL_output;
```

#### System Procedure SET_SHAREHOLDER_VOTES

`IMPLEMENT_SET_SHAREHOLDER_VOTES()` adds a system procedure invoked when `qpi.setShareholderVotes()` is called in another contract that is shareholder of your contract and wants to set shareholder votes in your contract.
It simply calls the user procedure `SetShareholderVotes`. Input and output are the same.

```C++
SET_SHAREHOLDER_VOTES()
{
    CALL(SetShareholderVotes, input, output);
}
```

The input and output of `SET_SHAREHOLDER_VOTES` are defined as follows in `qpi.h`:

```C++
	// Input of SET_SHAREHOLDER_VOTES system procedure (vote data)
	typedef ProposalMultiVoteDataV1 SET_SHAREHOLDER_VOTES_input;

	// Output of SET_SHAREHOLDER_VOTES system procedure (success flag)
	typedef bit SET_SHAREHOLDER_VOTES_output;
```


## Voting by Computors

Currently, the following contracts implement voting by computors:

- GQMPROP: General Quorum Proposals are made for deciding about inclusion of new contracts, weekly computor revenue donations, and other major strategic decisions related to the Qubic project.
- CCF: The Computor Controlled Fund is a contract for financing approved projects that contribute to expanding the capabilities, reach, or efficiency of the Qubic network. Projects are proposed by community members and selected through voting by the computors.

It is similar to shareholder voting and both share a lot of the code.
There are two major differences:

1. Each computor has exactly one vote. In shareholder voting, a shareholder often has multiple votes / shares.
2. The entities who are allowed to propose and vote aren't shareholders but computors.

Both is implemented in the `ProposersAndVotersT` by using `ProposalAndVotingByComputors` or `ProposalByAnyoneVotingByComputors` instead of `ProposalAndVotingByShareholders`.


## Proposal types

Each proposal type is composed of a class and a number of options. As an alternative to having N options (option votes), some proposal classes (currently the one to set a variable) may allow to vote with a scalar value in a range defined by the proposal (scalar voting).

The proposal type classes are defined in `QPI::ProposalTypes::Class`. The following are available at the moment:

```C++
    // Options without extra data. Supported options: 2 <= N <= 8 with ProposalDataV1.
    static constexpr uint16 GeneralOptions = 0;

    // Propose to transfer amount to address. Supported options: 2 <= N <= 5 with ProposalDataV1.
    static constexpr uint16 Transfer = 0x100;

    // Propose to set variable to a value. Supported options: 2 <= N <= 5 with ProposalDataV1; N == 0 means scalar voting.
    static constexpr uint16 Variable = 0x200;

    // Propose to set multiple variables. Supported options: 2 <= N <= 8 with ProposalDataV1
    static constexpr uint16 MultiVariables = 0x300;

    // Propose to transfer amount to address in a specific epoch. Supported options: 1 with ProposalDataV1.
    static constexpr uint16 TransferInEpoch = 0x400;
```

``QPI::ProposalType`` provides the following functions to work with proposal types:

```C++
    // Construct type from class + number of options (no checking if type is valid)
    uint16 type(uint16 cls, uint16 options);

    // Return option count for a given proposal type (including "no change" option),
    // 0 for scalar voting (no checking if type is valid).
    uint16 optionCount(uint16 proposalType);

    // Return class of proposal type (no checking if type is valid).
    uint16 cls(uint16 proposalType);

    // Check if given type is valid (supported by most comprehensive ProposalData class).
    bool isValid(uint16 proposalType);
```

For convenience, ``QPI::ProposalType`` also provides many proposal type constants, for example:

```C++
    // Set given variable to proposed value with options yes/no
    static constexpr uint16 VariableYesNo = Class::Variable | 2;

    // Set given variable to proposed value with two options of values and option "no change"
    static constexpr uint16 VariableTwoValues = Class::Variable | 3;

    // Set given variable to proposed value with three options of values and option "no change"
    static constexpr uint16 VariableThreeValues = Class::Variable | 4;

    // Set multiple variables with options yes/no (data stored by contract) -> result is histogram of options
    static constexpr uint16 MultiVariablesYesNo = Class::MultiVariables | 2;

    // Options yes and no without extra data -> result is histogram of options
    static constexpr uint16 YesNo = Class::GeneralOptions | 2;

    // Transfer given amount to address with options yes/no
    static constexpr uint16 TransferYesNo = Class::Transfer | 2;
```

See `qpi.h` for more.

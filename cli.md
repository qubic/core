./qubic-cli [basic config] [command] [command extra parameters]
-help print this message

Basic config:
	-conf <file>
		Specify configuration file. Relative paths will be prefixed by datadir location. See qubic.conf.example.
		Notice: variables in qubic.conf will be overrided by values on parameters.
	-seed <SEED>
		55-char seed for private key
	-nodeip <IPv4_ADDRESS>
		IP address of the target node for querying blockchain information (default: 127.0.0.1)
	-nodeport <PORT>
		Port of the target node for querying blockchain information (default: 21841)
	-scheduletick <TICK_OFFSET>
		Offset number of scheduled tick that will perform a transaction (default: 20)
	-force
		Do action although an error has been detected. Currently only implemented for proposals.
	-enabletestcontracts
		Enable test contract indices and names for commands using a contract index parameter. This flag has to be passed before the contract index/name. The node to connect to needs to have test contracts running.
	-print-only <base64 | hex>
        Print the raw transaction data without sending it to the network. Useful for offline signing or broadcasting later.
Commands:

[WALLET COMMANDS]
	-showkeys
		Generate identity, public key and private key from seed. Seed must be passed either from params or configuration file.
	-getbalance <IDENTITY>
		Balance of an identity (amount of qubic, number of in/out txs)
	-getasset <IDENTITY>
		Print a list of assets of an identity
	-queryassets <QUERY_TYPE> <QUERY_STING>
		Query and print assets information. Skip arguments to get detailed documentation.
	-gettotalnumberofassetshares <ISSUER_ID> <ASSET_NAME>
		Get total number of shares currently existing of a specific asset.
	-sendtoaddress <TARGET_IDENTITY> <AMOUNT>
		Perform a standard transaction to sendData <AMOUNT> qubic to <TARGET_IDENTITY>. A valid seed and node ip/port are required.
	-sendtoaddressintick <TARGET_IDENTITY> <AMOUNT> <TICK>
		Perform a standard transaction to sendData <AMOUNT> qubic to <TARGET_IDENTITY> in a specific <TICK>. A valid seed and node ip/port are required.

[QUTIL COMMANDS]
	-qutilsendtomanyv1 <FILE>
		Performs multiple transaction within in one tick. <FILE> must contain one ID and amount (space seperated) per line. Max 25 transaction. Fees apply! Valid seed and node ip/port are required.
	-qutilburnqubic <AMOUNT>
		Performs burning qubic, valid seed and node ip/port are required.
	-qutilburnqubicforcontract <AMOUNT> <CONTRACT_INDEX>
		Burns qubic for the specified contract index, valid seed and node ip/port are required.
	-qutilqueryfeereserve <CONTRACT_INDEX>
		Queries the amount of qubic in the fee reserve of the specified contract, valid node ip/port are required.
	-qutildistributequbictoshareholders <ISSUER_ID> <ASSET_NAME> <AMOUNT>
		Distribute QU among shareholders, transferring the same amount of QU for each share. The fee is proportional to the number of shareholders. The remainder that cannot be distributed equally is reimbursed.
	-qutilsendtomanybenchmark <DESTINATION_COUNT> <NUM_TRANSFERS_EACH>
		Sends <NUM_TRANSFERS_EACH> transfers of 1 qu to <DESTINATION_COUNT> addresses in the spectrum. Max 16.7M transfers total. Valid seed and node ip/port are required.
	-qutilcreatepoll <POLL_NAME> <POLL_TYPE> <MIN_AMOUNT> <GITHUB_LINK> <SEMICOLON_SEPARATED_ASSETS>
		Create a new poll. <POLL_NAME> is the poll's name (32 bytes), <POLL_TYPE> is 1 for Qubic or 2 for Asset, <MIN_AMOUNT> is the minimum vote amount, <GITHUB_LINK> is a 256-byte GitHub link. For Asset polls (type 2), provide a semicolon-separated list of assets in the format 'asset_name,issuer;asset_name,issuer'. Valid seed and node ip/port are required.
	-qutilvote <POLL_ID> <AMOUNT> <CHOSEN_OPTION>
		Vote in a poll. <POLL_ID> is the poll's ID, <AMOUNT> is the vote amount, and <CHOSEN_OPTION> is the selected option (0-63). Valid seed and node ip/port are required.
	-qutilgetcurrentresult <POLL_ID>
		Get the current results of a poll. <POLL_ID> is the poll's ID. Valid node ip/port are required.
	-qutilgetpollsbycreator <CREATOR_ADDRESS>
		Get polls created by a specific user. <CREATOR_ADDRESS> is the creator's identity. Valid node ip/port are required.
	-qutilgetcurrentpollid
		Get the current poll ID and list of active polls.
	-qutilgetpollinfo <POLL_ID>
		Get information about a specific poll by its ID.
	-qutilcancelpoll <POLL_ID>
		Cancel a poll by its ID. Only the poll creator can cancel it. Requires seed and node ip/port.
	-qutilgetfee
		Show current QUTIL fees.

[BLOCKCHAIN/PROTOCOL COMMANDS]
	-gettickdata <TICK_NUMBER> <OUTPUT_FILE_NAME>
		Get tick data and write it to a file. Use -readtickdata to examine the file. valid node ip/port are required.
	-getquorumtick <COMP_LIST_FILE> <TICK_NUMBER>
		Get quorum tick data, the summary of quorum tick will be printed, <COMP_LIST_FILE> is fetched by command -getcomputorlist. valid node ip/port are required.
	-getcomputorlist <OUTPUT_FILE_NAME>
		Get computor list of the current epoch. Feed this data to -readtickdata to verify tick data. valid node ip/port are required.
	-getnodeiplist
		Print a list of node ip from a seed node ip. Valid node ip/port are required.
	-gettxinfo <TX_ID>
		Get tx infomation, will print empty if there is no tx or invalid tx. valid node ip/port are required.
	-checktxontick <TICK_NUMBER> <TX_ID>
		Check if a transaction is included in a tick. valid node ip/port are required.
	-checktxonfile <TX_ID> <TICK_DATA_FILE>
		Check if a transaction is included in a tick (tick data from a file). valid node ip/port are required.
	-readtickdata <FILE_NAME> <COMPUTOR_LIST>
		Read tick data from a file, print the output on screen, COMPUTOR_LIST is required if you need to verify block data
	-sendcustomtransaction <TARGET_IDENTITY> <TX_TYPE> <AMOUNT> <EXTRA_BYTE_SIZE> <EXTRA_BYTE_IN_HEX>
		Perform a custom transaction (IPO, querying smart contract), valid seed and node ip/port are required.
	-dumpspectrumfile <SPECTRUM_BINARY_FILE> <OUTPUT_CSV_FILE>
		Dump spectrum file into csv.
	-dumpuniversefile <UNIVERSE_BINARY_FILE> <OUTPUT_CSV_FILE>
		Dump universe file into csv.
	-dumpcontractfile <CONTRACT_BINARY_FILE> <CONTRACT_ID> <OUTPUT_CSV_FILE>
		Dump contract file into csv. Current supported CONTRACT_ID: 1-QX
	-makeipobid <CONTRACT_INDEX> <NUMBER_OF_SHARE> <PRICE_PER_SHARE>
		Participating IPO (dutch auction). valid seed and node ip/port, CONTRACT_INDEX are required.
	-getipostatus <CONTRACT_INDEX>
		View IPO status. valid node ip/port, CONTRACT_INDEX are required.
	-getactiveipos
		View list of active IPOs in this epoch. valid node ip/port are required.
	-getsysteminfo
		View Current System Status. Includes initial tick, random mining seed, epoch info.

[NODE COMMANDS]
	-getcurrenttick
		Show current tick information of a node
	-sendspecialcommand <COMMAND_IN_NUMBER> 
		Perform a special command to node, valid seed and node ip/port are required.	
	-togglemainaux <MODE_0> <Mode_1>
		Remotely toggle Main/Aux mode on node, valid seed and node ip/port are required.	
		<MODE_0> and <MODE_1> value are: MAIN or AUX	
	-setsolutionthreshold <EPOCH> <SOLUTION_THRESHOLD> 
		Remotely set solution threshold for future epoch, valid seed and node ip/port are required.	
	-refreshpeerlist
		(equivalent to F4) Remotely refresh the peer list of node, all current connections will be closed after this command is sent, valid seed and node ip/port are required.	
	-forcenexttick
		(equivalent to F5) Remotely force next tick on node to be empty, valid seed and node ip/port are required.	
	-reissuevote
		(equivalent to F9) Remotely re-issue (re-send) vote on node, valid seed and node ip/port are required.	
	-sendrawpacket <DATA_IN_HEX> <SIZE>
		Send a raw packet to nodeip. Valid node ip/port are required.
	-synctime
		Sync node time with local time, valid seed and node ip/port are required. Make sure that your local time is synced (with NTP)!	
	-getminingscoreranking
		Get current mining score ranking. Valid seed and node ip/port are required.	
	-getvotecountertx <COMPUTOR_LIST_FILE> <TICK>
		Get vote counter transaction of a tick: showing how many votes per ID that this tick leader saw from (<TICK>-675-3) to (<TICK>-3)
	-setloggingmode <MODE>
		Set console logging mode: 0 disabled, 1 low computational cost, 2 full logging. Valid seed and node ip/port are required.
	-savesnapshot
		Remotely trigger saving snapshot, valid seed and node ip/port are required.
	-setexecutionfeemultiplier <NUMERATOR> <DENOMINATOR>
		Set the multiplier for the conversion of raw execution time to contract execution fees to ( NUMERATOR / DENOMINATOR ), valid seed and node ip/port are required.
	-getexecutionfeemultiplier
		Get the current multiplier for the conversion of raw execution time to contract execution fees, valid seed and node ip/port are required.

[SMART CONTRACT COMMANDS]
    -callcontractfunction <CONTRACT_INDEX> <CONTRACT_FUNCTION> <INPUT_FORMAT_STRING> <OUTPUT_FORMAT_STRING>
        Call a contract function of contract index and print the output. Valid node ip/port are required.
    -invokecontractprocedure <CONTRACT_INDEX> <CONTRACT_PROCEDURE> <AMOUNT> <INPUT_FORMAT_STRING>
        Invoke a procedure of contract index. Valid seed and node ip/port are required.
	-setshareholderproposal <CONTRACT_INDEX> <PROPOSAL_STRING>
		Set shareholder proposal in a contract. May overwrite existing proposal, because each seed can have only one proposal at a time. Costs a fee. You need to be shareholder of the contract.
		<PROPOSAL_STRING> is explained if there is a parsing error. Most contracts only support "Variable|2" (yes/no proposals to change state variable).
	-clearshareholderproposal <CONTRACT_INDEX>
		Clear own shareholder proposal in a contract. Costs a fee.
	-getshareholderproposals <CONTRACT_INDEX> <PROPOSAL_INDEX_OR_GROUP>
		Get shareholder proposal info from a contract.
		Either pass "active" to get proposals that are open for voting in the current epoch, or "finished" to get proposals of previous epochs not overwritten or cleared yet, or a proposal index.
	-shareholdervote <CONTRACT_INDEX> <PROPOSAL_INDEX> <VOTE_VALUE>
		Cast vote(s) for a shareholder proposal in the contract. You need to be shareholder of the contract.
		<VOTE_VALUE> may be a single value to set all your votes (one per share) to the same value.
		In this case, <VOTE_VALUE> is the option in range 0 ... N-1 or "none" (in usual case of option voting), or an arbitrary integer or "none" (if proposal is for scalar voting).
		<VOTE_VALUE> also may be a comma-separated list of pairs of count and value (for example: "3,0,10,1" meaning 3 votes for option 0 and 10 votes for option 1).
		If the total count is less than the number of shares you own, the remaining votes will be set to "none".
	-getshareholdervotes <CONTRACT_INDEX> <PROPOSAL_INDEX> [VOTER_IDENTITY]
		Get shareholder proposal votes of the contract. If VOTER_IDENTITY is skipped, identity of seed is used.
	-getshareholderresults <CONTRACT_INDEX> <PROPOSAL_INDEX>
		Get the current result of a shareholder proposal.

[QX COMMANDS]
	-qxgetfee
		Show current Qx fee.
	-qxissueasset <ASSET_NAME> <NUMBER_OF_UNIT> <UNIT_OF_MEASUREMENT> <NUM_DECIMAL>
		Create an asset via Qx contract.
	-qxtransferasset <ASSET_NAME> <ISSUER_IN_HEX> <NEW_OWNER_IDENTITY> <AMOUNT_OF_SHARE>
		Transfer an asset via Qx contract.
	-qxorder add/remove bid/ask [ISSUER (in qubic format)] [ASSET_NAME] [PRICE] [NUMBER_OF_SHARE]
		Set order on Qx.
	-qxgetorder entity/asset bid/ask [ISSUER/ENTITY (in qubic format)] [ASSET_NAME (NULL for requesting entity)] [OFFSET]
		Get orders on Qx
	-qxtransferrights <ASSET_NAME> <ISSUER_ID> <NEW_MANAGING_CONTRACT> <NUMBER_OF_SHARES>
		Transfer asset management rights of shares from QX to another contract.
		<NEW_MANAGING_CONTRACT> can be given as name or index.
		You need to own/possess the shares to do this (seed required).

[QTRY COMMANDS]
	-qtrygetbasicinfo
		Show qtry basic info from a node.
	-qtryissuebet
		Issue a bet (prompt mode)
	-qtrygetactivebet
		Show all active bet id.
	-qtrygetactivebetbycreator <BET_CREATOR_ID>
		Show all active bet id of an ID.
	-qtrygetbetinfo <BET_ID>
		Get meta information of a bet
	-qtrygetbetdetail <BET_ID> <OPTION_ID>
		Get a list of IDs that bet on <OPTION_ID> of the bet <BET_ID>
	-qtryjoinbet <BET_ID> <NUMBER_OF_BET_SLOT> <AMOUNT_PER_SLOT> <PICKED_OPTION>
		Join a bet
	-qtrypublishresult <BET_ID> <WIN_OPTION>
		(Oracle providers only) publish a result for a bet
	-qtrycancelbet <BET_ID>
		(Game operator only) cancel a bet

[GENERAL QUORUM PROPOSAL COMMANDS]
	-gqmpropsetproposal <PROPOSAL_STRING>
		Set proposal in general quorum proposals contract. May overwrite existing proposal, because each computor can have only one proposal at a time. For success, computor status is needed.
		<PROPOSAL_STRING> is explained if there is a parsing error.
	-gqmpropclearproposal
		Clear own proposal in general quorum proposals contract. For success, computor status is needed.
	-gqmpropgetproposals <PROPOSAL_INDEX_OR_GROUP>
		Get proposal info from general quorum proposals contract.
		Either pass "active" to get proposals that are open for voting in the current epoch, or "finished" to get proposals of previous epochs not overwritten or cleared yet, or a proposal index.
	-gqmpropvote <PROPOSAL_INDEX> <VOTE_VALUE>
		Vote for proposal in general quorum proposals contract.
		<VOTE_VALUE> is the option in range 0 ... N-1 or "none".
	-gqmpropgetvote <PROPOSAL_INDEX> [VOTER_IDENTITY]
		Get vote from general quorum proposals contract. If VOTER_IDENTITY is skipped, identity of seed is used.
	-gqmpropgetresults <PROPOSAL_INDEX>
		Get the current result of a proposal (general quorum proposals contract).
	-gqmpropgetrevdonation
		Get and print table of revenue donations applied after each epoch.

[CCF COMMANDS]
	-ccfsetproposal <PROPOSAL_STRING>
		Set proposal in computor controlled fund (CCF) contract. May overwrite existing proposal, because each seed can have only one proposal at a time. Costs a fee.
		<PROPOSAL_STRING> is explained if there is a parsing error. Only "Transfer|2" (yes/no transfer proposals) are allowed in CCF.
		For subscription proposals, append subscription parameters to PROPOSAL_STRING: |<WEEKS_PER_PERIOD>|<START_EPOCH>|<NUMBER_OF_PERIODS>. The AMOUNT in the transfer proposal is used as AMOUNT_PER_PERIOD.
		To cancel the active subscription, <AMOUNT> or <WEEKS_PER_PERIOD> or <NUMBER_OF_PERIODS> should be zero.
	-ccfclearproposal
		Clear own proposal in CCF contract. Costs a fee.
	-ccfgetproposals <PROPOSAL_INDEX_OR_GROUP>
		Get proposal info from CCF contract.
		Either pass "active" to get proposals that are open for voting in the current epoch, or "finished" to get proposals of previous epochs not overwritten or cleared yet, or a proposal index.
	-ccfgetsubscription <SUBSCRIPTION_DESTINATION>
		Get active subscription info for a specific destination from CCF contract.
	-ccfvote <PROPOSAL_INDEX> <VOTE_VALUE>
		Cast vote for a proposal in the CCF contract.
		<VOTE_VALUE> is the option in range 0 ... N-1 or "none".
	-ccfgetvote <PROPOSAL_INDEX> [VOTER_IDENTITY]
		Get vote from CCF contract. If VOTER_IDENTITY is skipped, identity of seed is used.
	-ccfgetresults <PROPOSAL_INDEX>
		Get the current result of a CCF proposal.
	-ccflatesttransfers
		Get and print latest transfers of CCF granted by quorum.
	-ccfgetregularpayments
		Get and print regular payments (subscription payments) made by CCF contract.

[QEARN COMMANDS]
	-qearnlock <LOCK_AMOUNT>
		lock the qu to Qearn SC.
	-qearnunlock <UNLOCKING_AMOUNT> <LOCKED_EPOCH>
		unlock the qu from Qearn SC, unlock the amount of <UNLOCKING_AMOUNT> that locked in the epoch <LOCKED_EPOCH>.
	-qearngetlockinfoperepoch <EPOCH>
		Get the info(Total locked amount, Total bonus amount) locked in <EPOCH>.
	-qearngetuserlockedinfo <IDENTITY> <EPOCH>
		Get the locked amount that the user <IDENTITY> locked in the epoch <EPOCH>.
	-qearngetstateofround <EPOCH>
		Get the status(not started, running, ended) of the epoch <EPOCH>.
	-qearngetuserlockstatus <IDENTITY>
		Get the status(binary number) that the user locked for 52 weeks.
	-qearngetunlockingstatus <IDENTITY>
		Get the unlocking history of the user.
	-qearngetstatsperepoch <EPOCH>
		Get the Stats(early unlocked amount, early unlocked percent) of the epoch <EPOCH> and Stats(total locked amount, average APY) of QEarn SC
	-qearngetburnedandboostedstats
		Get the Stats(burned amount and average percent, boosted amount and average percent, rewarded amount and average percent in QEarn SC) of QEarn SC
	-qearngetburnedandboostedstatsperepoch <EPOCH>
		Get the Stats(burned amount and percent, boosted amount and percent, rewarded amount and percent in epoch <EPOCH>) of QEarn SC

[QVAULT COMMANDS]
	-qvaultsubmitauthaddress <NEW_ADDRESS>
		Submit the new authaddress using multisig address.
	-qvaultchangeauthaddress <NUMBER_OF_CHANGED_ADDRESS>
		Change the authaddress using multisig address. <NUMBER_OF_CHANGED_ADDRESS> is the one of (1, 2, 3).
	-qvaultsubmitfees <NEW_QCAPHOLDER_PERMILLE> <NEW_REINVESTING_PERMILLE> <NEW_DEV_PERMILLE>
		Submit the new permilles for QcapHolders, Reinvesting, Development using multisig address. the sum of 3 permilles should be 970 because the permille of shareHolder is 30. 
	-qvaultchangefees <NEW_QCAPHOLDER_PERMILLE> <NEW_REINVESTING_PERMILLE> <NEW_DEV_PERMILLE>
		Change the permilles for QcapHolders, Reinvesting, Development using multisig address. the sum of 3 permilles should be 970 because the permille of shareHolder is 30. Get the locked amount that the user <IDENTITY> locked in the epoch <EPOCH>.
	-qvaultsubmitreinvestingaddress <NEW_ADDRESS>
		Submit the new reinvesting address using multisig address. 
	-qvaultchangereinvestingaddress <NEW_ADDRESS>
		Change the address using multisig address. <NEW_ADDRESS> should be already submitted by -qvaultsubmitreinvestingaddress command.
	-qvaultsubmitadminaddress <NEW_ADDRESS>
		Submit the admin address using multisig address.
	-qvaultchangeadminaddress <NEW_ADDRESS>
		Change the admin address using multisig address. <NEW_ADDRESS> should be already submitted by -qvaultsubmitadminaddress command.
	-qvaultgetdata
		Get the state data of smart contract. anyone can check the changes after using the any command.
	-qvaultsubmitbannedaddress <NEW_ADDRESS>
		Submit the banned address using multisig address.
	-qvaultsavebannedaddress <NEW_ADDRESS>
		Save the banned address using multisig address. <NEW_ADDRESS> should be already submitted by -qvaultsubmitbannedaddress command.
	-qvaultsubmitunbannedaddress <NEW_ADDRESS>
		Submit the unbanned address using multisig address.
	-qvaultsaveunbannedaddress <NEW_ADDRESS>
		Unban the <NEW_ADDRESS> using the multisig address. <NEW_ADDRESS> should be already submitted by -qvaultsaveunbannedaddress command.

[MSVAULT COMMANDS]
	-msvaultregistervault <REQUIRED_APPROVALS> <VAULT_NAME> <OWNER_ID_COMMA_SEPARATED>
			Register a vault. Vault's number of votes for proposal approval <REQUIRED_APPROVALS>, vault name (max 32 chars), and a list of owners (separated by commas). Fee applies.
	-msvaultdeposit <VAULT_ID> <AMOUNT>
			Deposit qubic into vault given vault ID.
	-msvaultreleaseto <VAULT_ID> <AMOUNT> <DESTINATION_IDENTITY>
			Request release qu to destination. Fee applies.
	-msvaultresetrelease <VAULT_ID>
			Reset release requests. Fee applies.
	-msvaultgetvaults <IDENTITY>
			Get list of vaults owned by IDENTITY.
	-msvaultgetreleasestatus <VAULT_ID>
			Get release status of a vault.
	-msvaultgetbalanceof <VAULT_ID>
			Get balance of a vault.
	-msvaultgetvaultname <VAULT_ID>
			Get vault name.
	-msvaultgetrevenueinfo
			Get MsVault revenue info.
	-msvaultgetfees
			Get MsVault fees.
	-msvaultgetvaultowners <VAULT_ID>
			Get MsVault owners given vault ID.

[QSWAP COMMANDS]
	-qswapgetfee
		Show current Qswap fees.
	-qswapissueasset <ASSET_NAME> <NUMBER_OF_UNIT> <UNIT_OF_MEASUREMENT> <NUM_DECIMAL>
		Create an asset via Qswap contract.
	-qswaptransferasset <ASSET_NAME> <ISSUER_IN_HEX> <NEW_OWNER_IDENTITY> <AMOUNT_OF_SHARE>
		Transfer an asset via Qswap contract.
	-qswapcreatepool <ASSET_NAME> <ISSUER_IN_HEX>
		Create an AMM pool via Qswap contract.
	-qswapgetpoolbasicstate <ASSET_NAME> <ISSUER_IN_HEX>
		Get the basic info of a pool, totol liquidity, qu reserved, asset reserved.

    -qswapaddliquidity <ASSET_NAME> <ISSUER_IN_HEX> <QU_AMOUNT_IN> <ASSET_AMOUNT_DESIRED> <QU_AMOUNT_MIN> <ASSET_AMOUNT_MIN>
    	Add liquidity with restriction to an AMM pool via Qswap contract.
    -qswapremoveliquidity <ASSET_NAME> <ISSUER_IN_HEX> <BURN_LIQUIDITY> <QU_AMOUNT_MIN> <ASSET_AMOUNT_MIN>
    	Remove liquidity with restriction from an AMM pool via Qswap contract.
	-qswapswapexactquforasset <ASSET_NAME> <ISSUER_IN_HEX> <QU_AMOUNT_IN> <ASSET_AMOUNT_OUT_MIN>
		Swap qu for asset via Qswap contract, only execute if asset_amount_out >= ASSET_AMOUNT_OUT_MIN.
	-qswapswapquforexactasset <ASSET_NAME> <ISSUER_IN_HEX> <ASSET_AMOUNT_OUT> <QU_AMOUNT_IN_MAX>
		Swap qu for asset via Qswap contract, only execute if qu_amount_in <= QU_AMOUNT_IN_MAX.
	-qswapswapexactassetforqu <ASSET_NAME> <ISSUER_IN_HEX> <ASSET_AMOUNT_IN> <QU_AMOUNT_OUT_MIN>
		Swap asset for qu via Qswap contract, only execute if qu_amount_out >= QU_AMOUNT_OUT_MIN.
	-qswapswapassetforexactqu <ASSET_NAME> <ISSUER_IN_HEX> <QU_AMOUNT_OUT> <ASSET_AMOUNT_IN_MAX>
		Swap asset for qu via Qswap contract, only execute if asset_amount_in <= ASSET_AMOUNT_IN_MAX.
	-qswapgetliquidityof <ASSET_NAME> <ISSUER_IN_HEX> [LIQUIDITY_STAKER(in qublic format)]
		Get the staker's liquidity in a pool.
	-qswapquote exact_qu_input/exact_qu_output/exact_asset_input/exact_asset_output <ASSET_NAME> <ISSUER_IN_HEX> <AMOUNT>
    	Quote amount_out/amount_in with given amount_in/amount_out.

[NOSTROMO COMMANDS]
	-nostromoregisterintier <TIER_LEVEL>
			Register in tier.
	-nostromologoutfromtier 
			Logout from tier.
	-nostromocreateproject <TOKEN_NAME> <SUPPLY_OF_TOKEN> <START_YEAR> <START_MONTH> <START_DAY> <START_HOUR> <END_YEAR> <END_MONTH> <END_DAY> <END_HOUR>
			Create a project with the specified token info and start and end date for voting.
	-nostromovoteinproject <PROJECT_INDEX> <DECISION>
			Vote in the project with <DECISION> in the <PROJECT_INDEX> -> if you want to vote with yes, it should be 1. otherwise it is 0.
	-nostromocreatefundraising <TOKEN_PRICE> <SALE_AMOUNT> <REQUIRED_FUND> <PROJECT_INDEX> 
			<FIRST_PHASE_START_YEAR> <FIRST_PHASE_START_MONTH> <FIRST_PHASE_START_DAY> <FIRST_PHASE_START_HOUR>
			<FIRST_PHASE_END_YEAR> <FIRST_PHASE_END_MONTH> <FIRST_PHASE_END_DAY> <FIRST_PHASE_END_HOUR>
			<SECOND_PHASE_START_YEAR> <SECOND_PHASE_START_MONTH> <SECOND_PHASE_START_DAY> <SECOND_PHASE_START_HOUR>
			<SECOND_PHASE_END_YEAR> <SECOND_PHASE_END_MONTH> <SECOND_PHASE_END_DAY> <SECOND_PHASE_END_HOUR>
			<THIRD_PHASE_START_YEAR> <THIRD_PHASE_START_MONTH> <THIRD_PHASE_START_DAY> <THIRD_PHASE_START_HOUR>
			<THIRD_PHASE_END_YEAR> <THIRD_PHASE_END_MONTH> <THIRD_PHASE_END_DAY> <THIRD_PHASE_END_HOUR>
			<LISTING_START_YEAR> <LISTING_START_MONTH> <LISTING_START_DAY> <LISTING_START_HOUR>
			<CLIFF_END_YEAR> <CLIFF_END_MONTH> <CLIFF_END_DAY> <CLIFF_END_HOUR>
			<VESTING_END_YEAR> <VESTING_END_MONTH> <VESTING_END_DAY> <VESTING_END_HOUR>
			<THRESHOLDS> <TGE> <NUMBER_OF_STEP_FOR_VESTING>
			Create a fundraising with the specified token and project infos.
	-nostromoinvestinproject <FUNDRAISING_INDEX> <INVESTMENT_AMOUNT>
			Invest in the fundraising.
	-nostromoclaimtoken <CLAIM_AMOUNT> <FUNDRAISING_INDEX>
			Claim your token from SC.
			If you invest in the fundraising and also it is the time for claiming, you can receive the token from SC.
	-nostromoupgradetierlevel <NEW_TIER_LEVEL>
			Upgrade your tierlevel to <NEW_TIER_LEVEL>
	-nostromotransfersharemanagementrights <TOKEN_NAME> <TOKEN_ISSUER> <NEW_MANAGEMENT_CONTRACT_INDEX> <AMOUNT_OF_TRANSFER>
			Transfer the share management right to <NEW_MANAGEMENT_CONTRACT_INDEX>
	-nostromogetstats
			Get the infos of SC(like total pool weight, epoch revenue, number of registers, number of projects, ...).
	-nostromogettierlevelbyuser <USER_ID>
			Get the tier_level for <USER_ID>.
	-nostromogetuservotestatus <USER_ID>
			Get the list of project index voted by <USER_ID>.
	-nostromochecktokencreatability <TOKEN_NAME>
			Check if the <TOKEN_NAME> can be issued by SC.
			If <TOKEN_NAME> is already created by SC, it can not be issued anymore.
	-nostromogetnumberofinvestedprojects <USER_ID>
			Get the number invested and project. you can check if the <USER_ID> can invest.
			The max number that can invest by one user at once in SC is 128 currently.
	-nostromogetprojectbyindex <PROJECT_INDEX>
			Get the infos of project.
	-nostromogetfundraisingbyindex <FUNDRAISING_INDEX>
			Get the infos of fundraising.
	-nostromogetprojectindexlistbycreator <USER_ID>
			Get the list of project that <USER_ID> created.
	-nostromogetinfouserinvested <INVESTOR_ADDRESS>
			Get the invseted infos(indexOfFundraising, InvestedAmount, ClaimedAmount).
	-nostromogetmaxclaimamount <INVESTOR_ADDRESS> <INDEX_OF_FUNDRAISING>
			Get the max claim amount at the moment.

[QBOND COMMANDS]
	-qbondstake <MILLIONS_AMOUNT>
			Stake QU and get MBNDxxx token for every million of QU.
	-qbondtransfer <IDENTITY> <EPOCH> <AMOUNT>
			Transfer <AMOUNT> of MBonds of specific <EPOCH> to new owner <IDENTITY>
	-qbondaddask <EPOCH> <PRICE> <AMOUNT>
			Add ask order of <AMOUNT> MBonds of <EPOCH> at <PRICE>
	-qbondremoveask <EPOCH> <PRICE> <AMOUNT>
			Remove <AMOUNT> MBonds of <EPOCH> from ask order at <PRICE>
	-qbondaddbid <EPOCH> <PRICE> <AMOUNT>
			Add bid order of <AMOUNT> MBonds of <EPOCH> at <PRICE>
	-qbondremovebid <EPOCH> <PRICE> <AMOUNT>
			Remove <AMOUNT> MBonds of <EPOCH> from bid order at <PRICE>
	-qbondburnqu <AMOUNT>
			Burn <AMOUNT> of qu by QBOND sc.
	-qbondupdatecfa <IDENTITY> <OPERATION>
			Only for admin! Update commission free addresses. <OPERATION> must be 0 to remove <IDENTITY> or 1 to add.
	-qbondgetfees
			Get fees of QBond sc.
	-qbondgetearnedfees
			Get earned fees by QBond sc.
	-qbondgetinfoperepoch <EPOCH>
			Get overall information about <EPOCH> (stakers amount, total staked, APY)
	-qbondgetorders <EPOCH> <ASKS_OFFSET> <BIDS_OFFSET>
			Get orders of <EPOCH> MBonds.
	-qbondgetuserorders <OWNER> <ASKS_OFFSET> <BIDS_OFFSET>
			Get MBonds orders owner by <OWNER>.
	-qbondtable
			Get info about APY of each MBond.
	-qbondgetusermbonds <OWNER>
			Get MBonds owned by the <OWNER>.
	-qbondgetcfa
			Get list of commission free addresses.

[TESTING COMMANDS]
	-testqpifunctionsoutput
		Test that output of qpi functions matches TickData and quorum tick votes for 15 ticks in the future (as specified by scheduletick offset). Requires the TESTEXA SC to be enabled.
	-testqpifunctionsoutputpast
		Test that output of qpi functions matches TickData and quorum tick votes for the last 15 ticks. Requires the TESTEXA SC to be enabled.
    -testgetincomingtransferamounts <B_OR_C>
        Get incoming transfer amounts from either TESTEXB ("B") or TESTEXC ("C"). Requires the TESTEXB and TESTEXC SCs to be
enabled.
    -testbidinipothroughcontract <B_OR_C> <CONTRACT_INDEX> <NUMBER_OF_SHARE> <PRICE_PER_SHARE>
        Bid in an IPO either as TESTEXB ("B") or as TESTEXC ("C"). Requires the TESTEXB and TESTEXC SCs to be enabled.

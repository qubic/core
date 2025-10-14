#pragma once

#include "assets/assets.h"

#include "contract_core/contract_exec.h"
#include "contract_core/qpi_spectrum_impl.h"


// Notify dest of incoming transfer if dest is a contract. This is for outside QPI context only!
void notifyContractOfIncomingTransfer(const m256i& source, const m256i& dest, long long amount, unsigned char type);

static m256i releasedPublicKeys[NUMBER_OF_COMPUTORS];
static long long releasedAmounts[NUMBER_OF_COMPUTORS];
static unsigned int numberOfReleasedEntities;


// Bid in contract IPO (caller has to ensure that contractIndex is in IPO phase).
// This deducts price * quantity QU. Bids that don't get shares are refunded.
// Returns number of bids registered or -1 if any invalid value is passed or the owned funds aren't sufficient.
// If the return value >= 0, the full amount has been deducted, but if return value < quantity it has been partially
// refunded.
// This can be either called through QPI (contract procedure running in contract processor) or through a transaction (tick processor).
// If called by QPI, you must pass QPI context.
static long long bidInContractIPO(long long price, unsigned short quantity, const m256i& sourcePublicKey, const int spectrumIndex, const unsigned int contractIndex, const QPI::QpiContextProcedureCall* qpiContext = nullptr)
{
    ASSERT(spectrumIndex >= 0);
    ASSERT(spectrumIndex == ::spectrumIndex(sourcePublicKey));
    ASSERT(contractIndex < contractCount);
    ASSERT(system.epoch == contractDescriptions[contractIndex].constructionEpoch - 1);

    long long registeredBids = -1;

    if (price > 0 && price <= MAX_AMOUNT / NUMBER_OF_COMPUTORS
        && quantity > 0 && quantity <= NUMBER_OF_COMPUTORS)
    {
        const long long amount = price * quantity;
        if (decreaseEnergy(spectrumIndex, amount))
        {
            const QuTransfer quTransfer = { sourcePublicKey, m256i::zero(), amount };
            logger.logQuTransfer(quTransfer);

            registeredBids = 0;
            numberOfReleasedEntities = 0;
            contractStateLock[contractIndex].acquireWrite();
            IPO* ipo = (IPO*)contractStates[contractIndex];
            for (unsigned int i = 0; i < quantity; i++)
            {
                if (price <= ipo->prices[NUMBER_OF_COMPUTORS - 1])
                {
                    unsigned int j;
                    for (j = 0; j < numberOfReleasedEntities; j++)
                    {
                        if (sourcePublicKey == releasedPublicKeys[j])
                        {
                            break;
                        }
                    }
                    if (j == numberOfReleasedEntities)
                    {
                        releasedPublicKeys[numberOfReleasedEntities] = sourcePublicKey;
                        releasedAmounts[numberOfReleasedEntities++] = price;
                    }
                    else
                    {
                        releasedAmounts[j] += price;
                    }
                }
                else
                {
                    unsigned int j;
                    for (j = 0; j < numberOfReleasedEntities; j++)
                    {
                        if (ipo->publicKeys[NUMBER_OF_COMPUTORS - 1] == releasedPublicKeys[j])
                        {
                            break;
                        }
                    }
                    if (j == numberOfReleasedEntities)
                    {
                        releasedPublicKeys[numberOfReleasedEntities] = ipo->publicKeys[NUMBER_OF_COMPUTORS - 1];
                        releasedAmounts[numberOfReleasedEntities++] = ipo->prices[NUMBER_OF_COMPUTORS - 1];
                    }
                    else
                    {
                        releasedAmounts[j] += ipo->prices[NUMBER_OF_COMPUTORS - 1];
                    }

                    ipo->publicKeys[NUMBER_OF_COMPUTORS - 1] = sourcePublicKey;
                    ipo->prices[NUMBER_OF_COMPUTORS - 1] = price;
                    j = NUMBER_OF_COMPUTORS - 1;
                    while (j
                        && ipo->prices[j - 1] < ipo->prices[j])
                    {
                        const m256i tmpPublicKey = ipo->publicKeys[j - 1];
                        const long long tmpPrice = ipo->prices[j - 1];
                        ipo->publicKeys[j - 1] = ipo->publicKeys[j];
                        ipo->prices[j - 1] = ipo->prices[j];
                        ipo->publicKeys[j] = tmpPublicKey;
                        ipo->prices[j--] = tmpPrice;
                    }

                    contractStateChangeFlags[contractIndex >> 6] |= (1ULL << (contractIndex & 63));
                    ++registeredBids;
                }
            }
            contractStateLock[contractIndex].releaseWrite();

            for (unsigned int i = 0; i < numberOfReleasedEntities; i++)
            {
                if (!releasedAmounts[i])
                    continue;
                increaseEnergy(releasedPublicKeys[i], releasedAmounts[i]);
                if (qpiContext)
                    qpiContext->__qpiNotifyPostIncomingTransfer(m256i::zero(), releasedPublicKeys[i], releasedAmounts[i], QPI::TransferType::ipoBidRefund);
                else
                    notifyContractOfIncomingTransfer(m256i::zero(), releasedPublicKeys[i], releasedAmounts[i], QPI::TransferType::ipoBidRefund);
                const QuTransfer quTransfer = { m256i::zero(), releasedPublicKeys[i], releasedAmounts[i] };
                logger.logQuTransfer(quTransfer);
            }
        }
    }

    return registeredBids;
}

// Finish all current IPOs at the end of the epoch. Can only be called from tick processor.
static void finishIPOs()
{
    for (unsigned int contractIndex = 1; contractIndex < contractCount; contractIndex++)
    {
        if (system.epoch == (contractDescriptions[contractIndex].constructionEpoch - 1) && contractStates[contractIndex])
        {
            contractStateLock[contractIndex].acquireRead();
            IPO* ipo = (IPO*)contractStates[contractIndex];
            long long finalPrice = ipo->prices[NUMBER_OF_COMPUTORS - 1];
            int issuanceIndex, ownershipIndex, possessionIndex;
            if (finalPrice)
            {
                if (!issueAsset(m256i::zero(), (char*)contractDescriptions[contractIndex].assetName, 0, CONTRACT_ASSET_UNIT_OF_MEASUREMENT, NUMBER_OF_COMPUTORS, QX_CONTRACT_INDEX, &issuanceIndex, &ownershipIndex, &possessionIndex))
                {
                    finalPrice = 0;
                }
            }
            numberOfReleasedEntities = 0;
            for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
            {
                if (ipo->prices[i] > finalPrice)
                {
                    unsigned int j;
                    for (j = 0; j < numberOfReleasedEntities; j++)
                    {
                        if (ipo->publicKeys[i] == releasedPublicKeys[j])
                        {
                            break;
                        }
                    }
                    if (j == numberOfReleasedEntities)
                    {
                        releasedPublicKeys[numberOfReleasedEntities] = ipo->publicKeys[i];
                        releasedAmounts[numberOfReleasedEntities++] = ipo->prices[i] - finalPrice;
                    }
                    else
                    {
                        releasedAmounts[j] += (ipo->prices[i] - finalPrice);
                    }
                }
                if (finalPrice)
                {
                    int destinationOwnershipIndex, destinationPossessionIndex;
                    transferShareOwnershipAndPossession(ownershipIndex, possessionIndex, ipo->publicKeys[i], 1, &destinationOwnershipIndex, &destinationPossessionIndex, true);
                }
            }
            for (unsigned int i = 0; i < numberOfReleasedEntities; i++)
            {
                ASSERT(releasedAmounts[i] > 0);
                increaseEnergy(releasedPublicKeys[i], releasedAmounts[i]);
                notifyContractOfIncomingTransfer(m256i::zero(), releasedPublicKeys[i], releasedAmounts[i], QPI::TransferType::ipoBidRefund);
                const QuTransfer quTransfer = { m256i::zero(), releasedPublicKeys[i], releasedAmounts[i] };
                logger.logQuTransfer(quTransfer);
            }
            contractStateLock[contractIndex].releaseRead();

            contractStateLock[0].acquireWrite();
            contractFeeReserve(contractIndex) = finalPrice * NUMBER_OF_COMPUTORS;
            contractStateLock[0].releaseWrite();
        }
    }
}

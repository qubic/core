# Revenue V2 — Epoch 204 Validation Results

V2 has been running in shadow mode alongside V1. Below are the results from **epoch 204** (676 computors, ~216,000 ticks), computed from the C++ node's end-of-epoch `.eoe` files.

**IPC (per-computor issuance):** 1,479,289,940 QU
**Parameters (V2):** SCALE=1024, bonus cap B=256, sliding window half=675 ticks

---

## 0. Model Overview

| Model | Formula | Key characteristics |
|---|---|---|
| **V1** | `IPC × txF × voteF × customMiningF / 1024³` | 3 multiplicative factors; vote in formula |
| **V2** | `IPC × M × (S² + B×E) / (S×(S+B)×S)` | Sliding window TX; custom mining additive bonus; uniform oracle |
| **V2+Oracle (weighted)** | Same as V2, `M = (17×txF + 3×oracleF) / 20` | Soft oracle incentive; zero oracle → 85% revenue |
| **V2+Oracle (multiplicative)** | Same as V2, `M = txF × oracleF / S` | Hard oracle enforcement; zero oracle → zero revenue |

**M (mandatory factor):**
- Weighted average: `M = (17×txFactor + 3×oracleFactor) / 20` — forgiving, soft oracle incentive
- Multiplicative: `M = txFactor × oracleFactor / S` — strict, zero in any mandatory = zero revenue
**E (custom mining bonus factor):** additive — adds up to +25% on top of base revenue

### V2 Formula Derivation

The V2 integer formula originates from a simple conceptual model (**Model C: Base × (1 + Bonus)**):

```
revenue = IPC × M × (1 + Bonus) / (1 + MaxBonus)
```

Where `MaxBonus = B/S` (cap) and `Bonus = MaxBonus × E/S = B×E/S²` (actual bonus, proportional to mining score E). The `/(1 + MaxBonus)` normalizes so revenue never exceeds IPC.

Converting to integer arithmetic (all factors in `[0, S]`, S=1024):

```
// Substitute Bonus = B×E/S², MaxBonus = B/S:
(1 + B×E/S²) / (1 + B/S)
= (S² + B×E)/S²  /  (S + B)/S       // express with common denominators
= (S² + B×E) / (S × (S + B))        // S²/S simplifies

score = M × (S² + B×E) / (S × (S + B))               // M in [0, S]
revenue = IPC × score / S                              // normalize M from [0,S] to [0,1]
        = IPC × M × (S² + B×E) / (S × (S+B) × S)     ← final V2 formula
```

### Component Breakdown

The formula decomposes as: **revenue = IPC × M × (base_share + bonus_share)**

```
revenue = IPC × (M/S) × [ S/(S+B) + B×E/(S×(S+B)) ]
                  ↑          ↑              ↑
              mandatory   base_share    bonus_share
```

| Component | Expression | Range | Effect |
|---|---|---|---|
| **M** (mandatory) | `(17×txF + 3×oracleF) / 20` | [0, S] | Scales everything — gates both base and bonus proportionally |
| **base_share** | `S/(S+B)` = 1024/1280 | fixed **80%** | Guaranteed revenue share when M is full, regardless of mining |
| **bonus_share** | `B×E/(S×(S+B))` | [0, **20%**] | Additional reward from custom mining; E=S → full 20% |
| **1/(1+MaxBonus)** | `S/(S+B)` = 1/1.25 | fixed | Normalization — ensures revenue ≤ IPC |

**Key behaviors:**
- Full mandatory, zero mining (M=S, E=0): revenue = IPC × 80% — the base floor
- Full mandatory, full mining (M=S, E=S): revenue = IPC × 100%
- Half mandatory, zero mining (M=S/2, E=0): revenue = IPC × 40% — M halves everything
- Half mandatory, full mining (M=S/2, E=S): revenue = IPC × 50% — full mining recovers the 20% bonus, but M still halves the total
- B controls the base/bonus split: larger B → more bonus room but lower floor

---

## 1. Real Data: V1 vs V2

### 1.1 Summary Statistics

| Metric | V1 | V2 | V2+Oracle |
|---|---|---|---|
| Mean (% IPC) | 94.4% | **99.0%** | 96.9% |
| Std deviation | 8.9% | 2.7% | 5.2% |
| Minimum (floor) | 68.5% | **84.5%** | 81.2% |
| Computors ≥ 99% IPC | 405 / 676 (60%) | **568 / 676 (84%)** | 381 / 676 (56%) |
| Computors 50–99% IPC | 271 / 676 (40%) | 108 / 676 (16%) | 295 / 676 (44%) |
| Computors < 50% IPC | **0** | **0** | **0** |

---

## 2. Three Problems V2 Fixes

### 2.1 Vote Factor Compounding (225 computors affected)

V1 multiplies TX × Vote × CustomMining. In epoch 204, 225 computors had vote scores below quorum — they lost 10–20% revenue purely from this multiplicative penalty. V2 removes vote from the formula entirely. Those 225 computors recover to near-full revenue.

| Group | # Computors | V1 avg revenue | V2 avg revenue | Difference |
|---|---|---|---|---|
| Both TX+Vote full | 354 | 99.8% | 99.9% | +0.1% |
| Only TX penalized | 97 | 98.8% | 99.0% | +0.2% |
| **Only Vote penalized** | **97** | **89.5%** | **99.9%** | **+10.4%** |
| **Both TX+Vote penalized** | **128** | **79.8%** | **95.9%** | **+16.1%** |

The 225 computors penalized by vote score in V1 lose 10–20% revenue relative to V2.
In V2, **vote is removed from the formula entirely** — those computors recover fully.

### 2.2 Custom Mining Shutout → Custom Mining Bonus

V1: custom mining is a **revenue gate** — if custom mining factor = 0, revenue = 0 (complete shutout). A computor that relays transactions perfectly but mines poorly gets severely punished.

V2: custom mining is an **additive bonus** (max +25% of IPC). Base revenue requires only TX relay and oracle participation. A computor with zero custom mining still earns **~80%** of IPC from relay alone.

| Custom Mining level (E) | V1 (custom mining alone drops) | V2 (additive bonus) |
|---|---|---|
| 100% (full) | 100% | 100% |
| 80% | 80% | 96% |
| 50% | 50% | 90% |
| 0% (no custom mining) | **0%** | **~80%** |

This matters because custom mining is hardware-dependent and partially outside operator control.
V2 ensures that **no computor is shut out** for not participating in custom mining.

### 2.3 TX Luck Variance

Each tick is assigned to one computor (`tick_number % 676`). TX traffic is bursty and uneven:

| Metric | Raw TX total per computor | Sliding window score |
|---|---|---|
| Mean | 5,790 TX | 327,603 |
| Std deviation | 4,366 | 80,299 |
| CV (stdev/mean) | **0.754** | **0.245** |
| Range (max/min) | **6.8×** | **2.3×** |

The sliding window **reduces the coefficient of variation by 67.5%** and compresses the range from 6.8× to 2.3×.

Instead of scoring only the ticks assigned to a computor, V2's sliding window collects TX from a `±675 tick` neighborhood (= ±1 full computor rotation). Each tick's score reflects the **local traffic context**, not just absolute TX count.

```
Raw approach (V1): computor 42 happens to get busy ticks → high score
                   computor 100 gets quiet ticks → low score
                   Result: luck dominates if traffic is bursty

Window approach (V2): computor 42's score normalized against its local window
                      computor 100 also normalized against its local window
                      Result: each computor measured relative to its traffic context
```

Per-tick data (epoch 204): mean TX/tick = 18.1, max = 1,021, with large variance.
Without smoothing, a computor assigned to the 1,021-TX tick scores ~56× more than one assigned to a 18-TX tick. The window averages this out.

---

## 3. Penalty Degradation

### 3.1 Single Factor Drop (others stay at 100%)

> Answers: "If only one thing goes wrong, how much revenue do I lose?"

**V1:** All three factors (TX, Vote, CustomMining) are **symmetric and multiplicative**.
Dropping any single factor = `pct%` revenue — **proportional (linear)**. No compounding from a single factor.

**V2:** Each sub-factor has a **protective floor** due to weighted M formula and additive E:

| Factor level | V1 TX=Vote=CustomMining (linear) | V2 TX-only (floor=15%) | V2 Oracle-only (floor=85%) | V2 CustomMining/E (floor≈80%) |
|---|---|---|---|---|
| 100% | 100.0% | 100.0% | 100.0% | 100.0% |
| 80% | 80.0% | 82.9% | 97.0% | 96.0% |
| 50% | 50.0% | 57.4% | 92.5% | 90.0% |
| 20% | 20.0% | 31.9% | 88.0% | 84.0% |
| **0%** | **0.0%** | **14.9%** | **85.0%** | **80.0%** |

**Why V2 has floors:**
- **TX-only floor = 15%**: M = (17×txF + 3×oracleF)/20 — even with TX=0, oracle's 15% weight keeps M at 15%.
- **Oracle-only floor = 85%**: TX has 85% weight in M — oracle dropout alone can only reduce M to 85%.
- **CustomMining/E floor = 80%**: When E=0, revenue = S/(S+B) = 1024/1280 ≈ **80%**. Custom mining bonus adds up to +25%, not a gate.

### 3.2 Two-Factor Compound Drop

> What happens when a computor struggles on two dimensions simultaneously?

| Factor level | V1 TX×Vote (customMining=100%) | V1 TX×CustomMining (vote=100%) | V2 M×E both drop |
|---|---|---|---|
| 100% | 100.0% | 100.0% | 100.0% |
| 80% | **64.0%** | **64.0%** | **76.8%** |
| 50% | **25.0%** | **25.0%** | **45.0%** |
| 20% | **4.0%** | **4.0%** | **16.7%** |
| 0% | 0.0% | 0.0% | 0.0% |

V1's two-factor compounding drops revenue to **64%** at 80% factor level — far harsher than V2's 76.8%.
At 50%, V1 punishes to **25%** while V2 only to **45%**.

### 3.3 All Factors Drop Together

> Worst case: everything degrades to the same level simultaneously.

| Factor level | V1 (txF×vF×cmF cubic) | V2 (M×E, additive) |
|---|---|---|
| 100% | 100.0% | 100.0% |
| 80% | **51.2%** | **76.8%** |
| 60% | **21.6%** | **52.4%** |
| 50% | **12.5%** | **45.0%** |
| 20% | **0.8%** | **16.7%** |
| 0% | 0.0% | 0.0% |

V1's three-factor cubic compounding is catastrophic: **80% performance → only 51.2% revenue**.
V2's additive custom mining bonus prevents this: **80% performance → 76.8% revenue**.

---

## 4. V2+Oracle Variant

Adding real oracle scores (15% weight in M) creates an oracle participation incentive. 73 computors with zero oracle participation see reduced revenue (floor drops from 84.5% to 81.2%).

This is intentional — the 15% oracle weight in M provides a **moderate incentive without being a gate**:
- Oracle = 0 → M drops by at most 15% → floor at ~81% IPC
- Oracle = full → full M → up to 100% IPC

---

## 5. Oracle Enforcement: Multiplicative Mandatory

### Problem with Weighted Average M

The current V2 formula uses a weighted average for the mandatory factor:

```
M = (17×txFactor + 3×oracleFactor) / 20
```

A computor with **zero oracle** but full TX still gets M = 17×1024/20 = **870** (85% of full M). Oracle is only a soft incentive — zero oracle costs at most 15% of revenue. This is too weak to enforce oracle participation.

### Solution: Switch M to Multiplicative

Instead of weighted average, combine mandatory factors multiplicatively:

```
M = txFactor × oracleFactor / S
```

Zero in **any** mandatory factor naturally zeros M → zero revenue. No explicit zero-gate needed — it's a structural property of multiplication.

The full formula becomes:

```
revenue = IPC × (txF × oracleF / S) × (S² + B×E) / (S × (S+B) × S)
        = IPC × txF × oracleF × (S² + B×E) / (S² × (S+B) × S)
```

### Comparison: Weighted Average vs Multiplicative M

| TX | Oracle | M (weighted avg) | Revenue (avg) | M (multiplicative) | Revenue (mult) |
|---|---|---|---|---|---|
| 1024 | 1024 | 1024 (100%) | 100% | 1024 (100%) | 100% |
| 1024 | 819 (80%) | 993 (97%) | 97% | 819 (80%) | 80% |
| 1024 | 512 (50%) | 947 (92%) | 92% | 512 (50%) | 50% |
| 1024 | 0 | 870 (85%) | **85%** | **0** | **0%** |
| 512 | 1024 | 589 (58%) | 58% | 512 (50%) | 50% |
| 512 | 512 | 512 (50%) | 50% | 256 (25%) | 25% |
| 0 | 1024 | 154 (15%) | 15% | **0** | **0%** |

**Key differences:**
- **Weighted average**: Forgiving — strong TX can cover weak oracle. Zero oracle still earns 85%.
- **Multiplicative**: Strict — each mandatory factor scales independently. Zero in any = zero revenue. Both factors must be healthy for full revenue.

### Trade-off

Multiplicative M brings back some of V1's compounding behavior for mandatory factors (TX × Oracle). A computor at 80% on both gets M = 0.8 × 0.8 = 64%, not 80%. However, this compounding only applies within mandatory — custom mining remains an additive bonus, not a multiplicative gate.

| TX | Oracle | M (multiplicative) | Full mining bonus | Final revenue |
|---|---|---|---|---|
| 100% | 100% | 100% | +20% | 100% |
| 80% | 80% | 64% | +12.8% | 76.8% |
| 80% | 100% | 80% | +16% | 96% |
| 100% | 80% | 80% | +16% | 96% |
| 50% | 50% | 25% | +5% | 30% |

The compounding is intentional for mandatory factors — it enforces that **both** TX relay and oracle participation are essential network duties. Custom mining remains a safe bonus on top.

### Existing Safeguard

The code already handles the case where NO computors have oracle activity — all get full oracle factor ([revenue.h:267-289](src/revenue.h#L267-L289)). Multiplicative M only penalizes zero-oracle computors when oracle queries exist but they didn't participate.

### Impact on Existing Computors

From epoch 204 data: 73 out of 676 computors had zero oracle participation. With multiplicative M, these 73 would receive **zero revenue** instead of ~81% IPC.

---

## 6. Summary

| Property | V1 | V2 | V2+Oracle (weighted) | V2+Oracle (multiplicative) |
|---|---|---|---|---|
| Revenue formula | Multiplicative (cubic) | Additive bonus | Additive bonus + oracle (avg M) | Additive bonus + oracle (mult M) |
| Mean revenue | 94.4% IPC | **99.0% IPC** | 96.9% IPC | — |
| Floor (worst case) | 68.5% IPC | **84.5% IPC** | 81.2% IPC | 0% (zero in any mandatory) |
| Vote in formula | Yes (penalizes 225 computors) | **No** | No | No |
| Custom mining shutout possible | **Yes** (0% if customMining=0) | No (80% floor) | No (80% floor) | No (80% floor) |
| Sliding window TX | No | **Yes** | Yes | Yes |
| TX luck variance | 6.8× range | **2.3× range** | 2.3× range | 2.3× range |
| Mandatory combination | All multiplicative | TX only | Weighted average | **Multiplicative** |
| Zero oracle penalty | None | None | −15% (floor ~81%) | **Zero revenue** |
| Both mandatory at 80% | — | — | M=97% → ~97% | M=64% → ~64% |
| Oracle enforcement | None | None | Soft | **Hard (structural)** |

**V2** eliminates V1's three main failure modes: vote compounding, custom mining shutout, and TX luck domination.
**V2+Oracle (weighted)** adds a soft oracle incentive (15% weight), but 73 computors with zero participation only lose ~15% — too weak to enforce oracle duties.
**V2+Oracle (multiplicative)** enforces oracle structurally: `M = txF × oracleF / S` naturally zeros revenue when any mandatory factor is zero. Custom mining remains an additive bonus — only mandatory factors compound.

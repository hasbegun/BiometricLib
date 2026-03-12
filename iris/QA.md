# Q&A Archive

## Q: Is OpenFHE justified for iris recognition? Can it match without decryption?

**Date**: 2026-03-02

### Context

In this project, once an iris is enrolled it's stored in designated storage with no modification — only delete and re-enroll. At verification time, the system must check whether a new iris matches an enrolled one. The question: can OpenFHE do this without decrypting the stored templates?

### Answer: Yes, FHE allows matching without decryption

**How it works in our implementation:**

```
ENROLLMENT (client-side):
  iris image → pipeline → PackedIrisCode → EncryptedTemplate::encrypt() → store ciphertext on server

VERIFICATION (split client/server):
  Client:  new iris → pipeline → PackedIrisCode → EncryptedTemplate::encrypt() → send to server
  Server:  EncryptedMatcher::match_encrypted(stored_ct, probe_ct) → encrypted_hd_result
           ↑ server NEVER sees plaintext iris code
  Client:  EncryptedMatcher::decrypt_result(encrypted_hd_result) → HD score → match/no-match
```

BFV arithmetic on encrypted binary values:
- **XOR** (bit disagreement): `(a - b)²` — works because `(0-1)² = (1-0)² = 1`, `(0-0)² = (1-1)² = 0`
- **AND** (valid mask bits): `a * b`
- **Summation**: `EvalSum` aggregates across 8192 SIMD slots

BFV is **exact** — encrypted HD matches plaintext HD exactly (no approximation error).

### Efficiency Analysis

| Operation | Plaintext | FHE (BFV) | Ratio |
|-----------|-----------|-----------|-------|
| Single HD comparison | ~1 μs (SIMD popcount) | ~200-500 ms | ~200,000x slower |
| Key generation | N/A | ~2-3 s | one-time |
| Template encryption | N/A | ~100-200 ms | per template |
| Rotation matching (±15 shifts) | ~30 μs | ~6-15 s | 31 encrypt + match cycles |

- **1:1 verification** (claim identity X, check against template X): FHE is viable. One comparison, 200-500ms acceptable.
- **1:N identification** (find who you are among N): FHE is impractical. 1M enrolled × 300ms ≈ 83 hours.

### Rotation Problem

Current implementation rotates probe in **plaintext**, encrypts each rotation separately, then matches:
```
for shift in [-N..+N]:
    rotated_probe = probe.rotate(shift)       // plaintext
    encrypted_probe = encrypt(rotated_probe)   // re-encrypt each
    hd = match_encrypted(gallery_ct, encrypted_probe)
```
This means 31 separate encryptions + 31 homomorphic matches for ±15 shifts. BFV slot rotation (`EvalRotate`) doesn't map cleanly to iris code column rotation.

### Alternatives

**1. Trusted Execution Environments (TEE) — Intel SGX / ARM TrustZone / AWS Nitro**
- Near-native speed (~1x vs 200,000x for FHE)
- Trust model: hardware manufacturer (not math)
- Side-channel attacks are real (Spectre, Plundervolt on SGX)
- Best for: high-throughput 1:N matching where you trust hardware

**2. Secure Multi-Party Computation (MPC / Secret Sharing)**
- Split template: `T = S₁ ⊕ S₂`, store shares on separate servers
- ~1,000-10,000x slowdown (10-100x faster than FHE)
- Requires multiple non-colluding servers
- Best for: medium throughput where you can't trust a single server

**3. Fuzzy Commitment / Cancelable Biometrics**
- `hash(iris_code ⊕ random_codeword)` → stored token
- Near-native speed
- Weaker security guarantees, some published schemes broken
- Best for: speed-critical deployments with acceptable risk

**4. Hybrid: FHE/AES for Storage + TEE for Matching**
- Store encrypted at rest (survives storage breach)
- Decrypt inside TEE enclave → match at native speed → return only yes/no
- Best of both: encrypted storage + fast matching

### When FHE IS Justified

1. **Don't trust the server operator** — not even with hardware attestation
2. **1:1 verification is the primary use case** — one comparison per request is tolerable
3. **Regulatory requirement** — biometric data must never exist in plaintext on server
4. **Defense in depth** — even full server compromise (OS, hypervisor, memory dump) leaves templates protected

### When FHE is NOT the Best Choice

1. **1:N identification at scale** — computationally infeasible
2. **Low latency critical** — sub-10ms matching (e.g., turnstile speed)
3. **Hardware enclaves acceptable** — TEE gives ~200,000x better performance

### Bottom Line

FHE's unique value: security is **mathematical**, not dependent on trusting hardware or non-collusion. For production iris systems at scale, most deployments use **TEE or hybrid** approaches. FHE is best as a **premium privacy tier** for 1:1 verification where the user claims an identity and you confirm against one stored template.

---

## Q: How exactly does OpenFHE encrypt iris codes and verify identity without decryption?

**Date**: 2026-03-02

### 1. What Is an Iris Code?

An iris code is a compact binary representation of the unique texture patterns in a human iris. After the image processing pipeline (segmentation, normalization, Gabor filtering, binarization), each iris produces:

```
PackedIrisCode:
  code_bits: 128 × uint64_t = 8192 bits   (the iris texture pattern)
  mask_bits: 128 × uint64_t = 8192 bits   (which bits are reliable)
```

The 8192 bits come from **16 rows × 256 columns × 2 channels** (real and imaginary parts of Gabor filter responses). Each bit is either 0 or 1. The mask marks which bits are trustworthy (not occluded by eyelids, reflections, etc.).

Two irises from the **same person** will have a small Hamming Distance (HD ≈ 0.2-0.3), while two irises from **different people** will have HD ≈ 0.45-0.5 (near random chance for binary data).

### 2. Plaintext Hamming Distance (What We're Trying to Replicate Under Encryption)

In plaintext, HD is trivial — just XOR and count:

```
Numerator   = popcount(code_A XOR code_B AND mask_A AND mask_B)
Denominator = popcount(mask_A AND mask_B)
HD = Numerator / Denominator
```

This takes ~1 microsecond with SIMD. The challenge: can we compute this **without ever seeing code_A or code_B in plaintext on the server**?

### 3. The BFV Encryption Scheme — What It Is

OpenFHE implements the **Braun-Fan-Vercauteren (BFV)** scheme, a **lattice-based** homomorphic encryption system. The key insight:

**BFV encrypts integers into polynomial rings, and arithmetic on ciphertexts corresponds to arithmetic on plaintexts.**

Specifically, for our parameters:

| Parameter | Value | Why |
|-----------|-------|-----|
| `plaintext_modulus` | 65537 | A prime; arithmetic happens mod this value. Since we only use 0 and 1, any prime > 1 works |
| `mult_depth` | 2 | We need at most 2 multiplications in sequence: `(a-b)² × mask` |
| `ring_dimension` | 16384 | Size of the polynomial ring; determines security level |
| `security_level` | 128-bit | Post-quantum secure (lattice problems are believed quantum-hard) |
| `slot_count` | 8192 | ring_dim / 2 = number of integers we can pack into one ciphertext |

The magical property: **8192 slots = exactly the number of bits in one iris code.** Each iris bit gets its own SIMD slot.

### 4. How Encryption Works — Step by Step

#### 4.1 Key Generation (`FHEContext::generate_keys()`)

```cpp
// Creates the mathematical structures (from fhe_context.cpp)
CCParams<CryptoContextBFVRNS> cc_params;
cc_params.SetPlaintextModulus(65537);
cc_params.SetMultiplicativeDepth(2);
cc_params.SetRingDim(16384);

cc_ = GenCryptoContext(cc_params);
cc_->Enable(PKE);          // Public Key Encryption
cc_->Enable(KEYSWITCH);    // Key switching (for rotation)
cc_->Enable(LEVELEDSHE);   // Leveled Somewhat Homomorphic Encryption
cc_->Enable(ADVANCEDSHE);  // Advanced ops (EvalSum, EvalRotate)

key_pair_ = cc_->KeyGen();                 // Public + Secret key pair
cc_->EvalMultKeyGen(key_pair_.secretKey);  // Relinearization key (for multiplication)
cc_->EvalSumKeyGen(key_pair_.secretKey);   // Summation key (for EvalSum)
cc_->EvalRotateKeyGen(key_pair_.secretKey, rotation_indices);  // Rotation keys
```

This produces:
- **Public key**: anyone can encrypt with it (the enrollment device, the client)
- **Secret key**: only the key holder can decrypt (stays on client, never on server)
- **Evaluation keys**: enable the server to perform computations on ciphertexts **without learning the plaintext**

#### 4.2 Template Encryption (`EncryptedTemplate::encrypt()`)

The iris code's 8192 bits are unpacked into individual integers (0 or 1), one per SIMD slot:

```cpp
// From encrypted_template.cpp — bits_to_slots()
std::vector<int64_t> slots(slot_count, 0);  // 8192 slots
for (size_t i = 0; i < 8192; ++i) {
    size_t word = i / 64;
    size_t bit = i % 64;
    slots[i] = static_cast<int64_t>((bits[word] >> bit) & 1ULL);
}
// slots = [0, 1, 1, 0, 1, 0, 0, 1, ...] — 8192 individual bit values
```

Then each slot vector is encrypted into a single ciphertext (a polynomial in the ring):

```cpp
auto code_pt = ctx.context()->MakePackedPlaintext(code_slots);
auto mask_pt = ctx.context()->MakePackedPlaintext(mask_slots);

code_ct_ = ctx.context()->Encrypt(ctx.public_key(), code_pt);
mask_ct_ = ctx.context()->Encrypt(ctx.public_key(), mask_pt);
```

**What the ciphertext looks like**: a pair of polynomials of degree 16383, with coefficients that are large integers (hundreds of bits). The original 0/1 values are buried in the algebraic structure of the polynomial ring, protected by a hard lattice problem (Ring-LWE).

**One iris template = 2 ciphertexts** (one for code, one for mask). Each ciphertext is ~500KB serialized.

#### 4.3 What the Server Stores

```
/enrolled_templates/
  user_001/
    code_ct.bin    (~500 KB — encrypted code bits, polynomial coefficients)
    mask_ct.bin    (~500 KB — encrypted mask bits, polynomial coefficients)
```

These files contain **only ciphertext**. Without the secret key, they are computationally indistinguishable from random data. Even with unlimited computing power, extracting the original iris code requires solving the Ring-LWE problem, which is believed to be hard even for quantum computers.

### 5. How Matching Works Without Decryption — The Core Magic

#### 5.1 The Algebraic Identity

For binary values (0 or 1), these equivalences hold:

| Plaintext Operation | Algebraic Equivalent | Why |
|---------------------|---------------------|-----|
| `a XOR b` | `(a - b)²` | `(0-0)²=0, (1-1)²=0, (0-1)²=1, (1-0)²=1` |
| `a AND b` | `a × b` | `0×0=0, 0×1=0, 1×0=0, 1×1=1` |
| `sum(vector)` | `EvalSum` | Adds all slots together into slot 0 |

These are all operations that BFV supports homomorphically — addition, subtraction, multiplication, and rotation/summation of SIMD slots.

#### 5.2 Server-Side Computation (`EncryptedMatcher::match_encrypted()`)

The server receives two encrypted templates and computes HD **entirely on ciphertexts**:

```cpp
// From encrypted_matcher.cpp — this is what the SERVER executes
Result<EncryptedResult> EncryptedMatcher::match_encrypted(
    const FHEContext& ctx,
    const EncryptedTemplate& probe,
    const EncryptedTemplate& gallery) {

    const auto& cc = ctx.context();
    const auto batch_size = static_cast<uint32_t>(ctx.slot_count());

    // Step 1: XOR via (a - b)²
    // Slot-wise: each of 8192 slots independently computes (probe_bit - gallery_bit)
    auto ct_diff = cc->EvalSub(probe.code_ct(), gallery.code_ct());
    // ct_diff encrypts: [p₀-g₀, p₁-g₁, p₂-g₂, ..., p₈₁₉₁-g₈₁₉₁]

    // Square it: slot-wise (pᵢ - gᵢ)²
    auto ct_xor = cc->EvalMult(ct_diff, ct_diff);
    // ct_xor encrypts: [XOR₀, XOR₁, XOR₂, ..., XOR₈₁₉₁]
    // where XORᵢ = 1 if bits disagree, 0 if they agree

    // Step 2: AND masks
    auto ct_mask = cc->EvalMult(probe.mask_ct(), gallery.mask_ct());
    // ct_mask encrypts: [m₀, m₁, m₂, ..., m₈₁₉₁]
    // where mᵢ = 1 only if BOTH masks are valid at position i

    // Step 3: Masked XOR — only count disagreements where both masks are valid
    auto ct_masked = cc->EvalMult(ct_xor, ct_mask);
    // ct_masked encrypts: [XOR₀×m₀, XOR₁×m₁, ..., XOR₈₁₉₁×m₈₁₉₁]

    // Step 4: Sum all 8192 slots into slot 0
    auto ct_numerator = cc->EvalSum(ct_masked, batch_size);
    // ct_numerator encrypts in slot 0: Σ(XORᵢ × mᵢ) = number of disagreeing valid bits

    auto ct_denominator = cc->EvalSum(ct_mask, batch_size);
    // ct_denominator encrypts in slot 0: Σ(mᵢ) = total number of valid bits

    return EncryptedResult{ct_numerator, ct_denominator};
}
```

**What the server sees during this computation**: polynomial coefficients being added, multiplied, and rotated. At no point can the server extract individual bit values, the XOR pattern, or even the final HD score. The result is **two encrypted integers** (numerator and denominator) that only the secret key holder can read.

**Multiplicative depth budget**: The computation uses exactly 2 multiplications in sequence:
1. `(a-b) × (a-b)` — depth 1
2. `xor_result × mask` — depth 2

This matches our `mult_depth = 2` parameter. BFV noise grows with each multiplication; at depth 2, the noise is still far below the decryption threshold, so the result decrypts correctly.

#### 5.3 Client-Side Decryption (`EncryptedMatcher::decrypt_result()`)

The server sends back the two encrypted integers. Only the client can decrypt:

```cpp
// From encrypted_matcher.cpp — this is what the CLIENT executes
Result<double> EncryptedMatcher::decrypt_result(
    const FHEContext& ctx,
    const EncryptedResult& result) {

    lbcrypto::Plaintext pt_num, pt_den;

    // Decrypt with secret key (only client has this)
    ctx.context()->Decrypt(ctx.secret_key(), result.ct_numerator, &pt_num);
    ctx.context()->Decrypt(ctx.secret_key(), result.ct_denominator, &pt_den);

    pt_num->SetLength(1);  // Only slot 0 has the sum
    pt_den->SetLength(1);

    auto num = pt_num->GetPackedValue()[0];  // e.g., 1847
    auto den = pt_den->GetPackedValue()[0];  // e.g., 7340

    if (den == 0) return 1.0;

    return static_cast<double>(num) / static_cast<double>(den);
    // HD = 1847 / 7340 = 0.2516... → MATCH (< 0.35 threshold)
}
```

**BFV is exact for integers**: unlike CKKS (which works with approximate real numbers), BFV decryption recovers the **exact** integer result. The encrypted HD equals the plaintext HD with zero error. This is proven by our tests:

```cpp
// From test_encrypted_matcher.cpp
TEST(EncryptedMatcher, MatchesPlaintext) {
    // ... setup ...
    auto enc_hd = EncryptedMatcher::match(ctx, code_a, code_b);
    auto plain_hd = plain_match(code_a, code_b);
    EXPECT_DOUBLE_EQ(*enc_hd, plain_hd);  // Exact equality, not approximate
}
```

### 6. What Each Party Knows

| Entity | Knows | Does NOT Know |
|--------|-------|---------------|
| **Client (enrollment device)** | Own iris code (plaintext), public key, secret key | Other users' iris codes |
| **Server (storage + compute)** | Encrypted templates (ciphertext), evaluation keys, encrypted HD result | Any plaintext iris code, the HD score, match/no-match decision |
| **Client (verification device)** | Own probe iris (plaintext), public key, secret key, HD score | Gallery iris codes (only sees the encrypted HD result) |

The server is a **pure computation engine**. It stores ciphertexts, performs polynomial arithmetic, and returns encrypted results. It learns nothing about:
- Whether two templates match
- How similar or different they are
- Any individual bit of any iris code
- Even how many valid mask bits exist

### 7. Why This Is Mathematically Secure

The security rests on the **Ring Learning With Errors (Ring-LWE)** problem:

```
Ciphertext = (a, b) where:
  a = random polynomial in Rq
  b = a·s + e + Δ·m   (mod q)

  s = secret key (a polynomial)
  e = small "error" polynomial (Gaussian noise)
  Δ = scaling factor = q/t
  m = plaintext message
  q = ciphertext modulus (very large)
  t = plaintext modulus (65537)
```

**To break it**, an attacker must solve: given `a` and `b = a·s + e + Δ·m`, find `s` (or `m`).

This is equivalent to finding the closest lattice vector in a high-dimensional lattice — a problem for which:
- No polynomial-time classical algorithm is known
- No polynomial-time quantum algorithm is known (unlike RSA/ECC which fall to Shor's algorithm)
- The best known attacks are exponential in the ring dimension

Our `ring_dimension = 16384` provides **128-bit post-quantum security** (`HEStd_128_classic`), meaning an attacker would need approximately 2^128 operations — more than the estimated number of atoms in the observable universe.

### 8. The Complete Verification Flow (End-to-End)

```
┌─────────────────────────────────────────────────────────────────┐
│                        ENROLLMENT                               │
│                                                                 │
│  Client Device:                                                 │
│    1. Capture iris image (IR camera)                            │
│    2. Run pipeline: image → PackedIrisCode (8192 code + mask)   │
│    3. Generate FHE keys (public_key, secret_key, eval_keys)     │
│    4. Encrypt: PackedIrisCode → EncryptedTemplate (2 ciphertexts)│
│    5. Send to server: {user_id, code_ct, mask_ct, eval_keys}    │
│    6. Securely store secret_key locally (never leaves device)   │
│                                                                 │
│  Server:                                                        │
│    7. Store ciphertexts + eval_keys indexed by user_id          │
│       (server has NO ability to decrypt — no secret_key)        │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│                       VERIFICATION                              │
│                                                                 │
│  Client Device:                                                 │
│    1. Capture new iris image                                    │
│    2. Run pipeline: image → PackedIrisCode (probe)              │
│    3. Encrypt probe with public_key → EncryptedTemplate         │
│    4. Send to server: {claimed_user_id, probe_code_ct,          │
│                        probe_mask_ct}                           │
│                                                                 │
│  Server (NEVER decrypts anything):                              │
│    5. Load stored {gallery_code_ct, gallery_mask_ct} for user   │
│    6. Compute on ciphertexts:                                   │
│       ct_diff = EvalSub(probe_code_ct, gallery_code_ct)         │
│       ct_xor  = EvalMult(ct_diff, ct_diff)                     │
│       ct_mask = EvalMult(probe_mask_ct, gallery_mask_ct)        │
│       ct_num  = EvalSum(EvalMult(ct_xor, ct_mask))              │
│       ct_den  = EvalSum(ct_mask)                                │
│    7. Send encrypted result: {ct_num, ct_den} back to client    │
│                                                                 │
│  Client Device:                                                 │
│    8. Decrypt with secret_key:                                  │
│       numerator   = Decrypt(ct_num) → e.g., 1847                │
│       denominator = Decrypt(ct_den) → e.g., 7340                │
│    9. Compute HD = 1847 / 7340 = 0.2516                         │
│   10. Decision: HD < 0.35 → MATCH (identity verified)           │
│                                                                 │
│  What the server learned: NOTHING.                              │
│  Not the iris codes, not the HD, not even match/no-match.       │
└─────────────────────────────────────────────────────────────────┘
```

### 9. Why the Stored Iris Never Needs Decryption

The key insight is that **Hamming distance is a simple arithmetic function** — it only needs subtraction, multiplication, and addition. These are exactly the operations that BFV supports homomorphically.

The server doesn't need to "understand" the iris code to compute HD. It just performs polynomial arithmetic that, by the mathematical structure of the BFV scheme, corresponds to the HD computation on the underlying plaintext. The server is essentially a blind calculator:

- It doesn't know what numbers it's adding (they're encrypted)
- It doesn't know what the result is (it's encrypted)
- It just follows the polynomial arithmetic rules
- The result, when decrypted by the key holder, is **provably correct**

This is why the enrolled template **never needs to be decrypted on the server**:

1. **At rest**: stored as polynomial coefficients — computationally indistinguishable from random data
2. **During matching**: manipulated as polynomials — the server performs blind arithmetic
3. **After matching**: the encrypted result is sent to the client — the server discards it
4. **On deletion**: the ciphertext files are deleted; without the secret key, no information can be recovered even from backups

### 10. What Happens If the Server Is Fully Compromised?

Even if an attacker gains:
- Root access to the server
- Full memory dumps during matching
- All stored ciphertext files
- All evaluation keys (EvalMult, EvalSum, EvalRotate)

They **still cannot**:
- Decrypt any iris template (requires secret key, which is on the client)
- Determine whether two people match (the HD result is encrypted)
- Extract any single iris bit from the ciphertext
- Distinguish a real ciphertext from random data (IND-CPA security)

The evaluation keys allow computation on ciphertexts but provide **zero information** about the plaintext. This is the fundamental separation that makes FHE unique: the ability to compute ≠ the ability to decrypt.

---

## Q: How are the cryptographic keys managed for FHE encryption and decryption?

**Date**: 2026-03-02

### Overview

BFV homomorphic encryption uses **five distinct key types**, each with a different purpose, owner, and security requirement. Understanding which key goes where — and why — is essential to understanding the trust model.

### 1. The Five Key Types

| Key | Generated By | Held By | Purpose | Leaking It Means... |
|-----|-------------|---------|---------|---------------------|
| **Public Key** | Client | Client + Server | Encrypt plaintext → ciphertext | Nothing — it's public by design |
| **Secret Key** | Client | Client ONLY | Decrypt ciphertext → plaintext | Full compromise: attacker can read all templates |
| **EvalMult Key** (relinearization) | Client (from secret key) | Server | Allow ciphertext × ciphertext multiplication | Nothing useful — cannot decrypt, only enables computation |
| **EvalSum Key** | Client (from secret key) | Server | Sum across SIMD slots in a ciphertext | Nothing useful — cannot decrypt |
| **EvalRotate Keys** | Client (from secret key) | Server | Rotate SIMD slots (barrel shifts) | Nothing useful — cannot decrypt |

The last three (EvalMult, EvalSum, EvalRotate) are collectively called **evaluation keys**. They are mathematically derived from the secret key but do **not** reveal it — they are one-way constructions.

### 2. Key Generation — What Happens in Code

```cpp
// From fhe_context.cpp — FHEContext::generate_keys()
Result<void> FHEContext::generate_keys() {
    // Step 1: Generate the public/secret key pair
    // The secret key is a polynomial with small coefficients (ternary: -1, 0, +1)
    // The public key is derived: pk = (a·sk + e, -a) where e is small noise
    key_pair_ = cc_->KeyGen();
    // key_pair_.publicKey  — safe to share
    // key_pair_.secretKey  — MUST stay on client

    // Step 2: EvalMult key (relinearization)
    // After multiplying two ciphertexts, the result has 3 polynomial components
    // instead of 2. Relinearization reduces it back to 2, keeping ciphertext
    // size constant. This key is derived FROM the secret key but does NOT
    // expose it — it's a one-way key-switching matrix.
    cc_->EvalMultKeyGen(key_pair_.secretKey);

    // Step 3: EvalSum key
    // Enables summing all 8192 SIMD slots into slot 0.
    // Internally, this is a series of slot rotations + additions.
    cc_->EvalSumKeyGen(key_pair_.secretKey);

    // Step 4: EvalRotate keys (power-of-2 barrel shifts)
    // Generate rotation keys for ±1, ±2, ±4, ±8, ..., ±4096 positions.
    // Any arbitrary rotation can be composed from these power-of-2 rotations.
    // Used by EvalSum internally and available for explicit slot manipulation.
    std::vector<int32_t> rotation_indices;
    const auto slots = static_cast<int32_t>(slot_count_);
    for (int32_t r = 1; r < slots; r *= 2) {
        rotation_indices.push_back(r);   // +1, +2, +4, ..., +4096
        rotation_indices.push_back(-r);  // -1, -2, -4, ..., -4096
    }
    cc_->EvalRotateKeyGen(key_pair_.secretKey, rotation_indices);
    // 26 rotation keys total (13 positive + 13 negative powers of 2)

    has_keys_ = true;
    return {};
}
```

### 3. Key Distribution Model

```
┌────────────────────────────────────────────────────────────────────────┐
│                     KEY GENERATION (Client Device)                     │
│                                                                        │
│   FHEContext::create() → BFV crypto context (parameters only)          │
│   FHEContext::generate_keys() → all 5 key types                        │
│                                                                        │
│   ┌─────────────────────┐    ┌──────────────────────────────────┐      │
│   │  STAYS ON CLIENT    │    │  SENT TO SERVER                  │      │
│   │                     │    │                                  │      │
│   │  - Secret Key       │    │  - Public Key                    │      │
│   │    (decryption)     │    │  - EvalMult Key (relinearize)    │      │
│   │                     │    │  - EvalSum Key (slot summation)  │      │
│   │                     │    │  - EvalRotate Keys (barrel shift)│      │
│   └─────────────────────┘    └──────────────────────────────────┘      │
│         NEVER leaves               Sent once at enrollment             │
│         the device                 Stored alongside ciphertexts        │
└────────────────────────────────────────────────────────────────────────┘
```

### 4. Why Evaluation Keys Don't Compromise the Secret Key

This is the most counter-intuitive part. The evaluation keys are **derived from** the secret key, so why can't an attacker reverse-engineer it?

**The relinearization key** is constructed as:

```
EvalMultKey = { (aᵢ·sk + eᵢ + sk²·baseⁱ, -aᵢ) }  for i = 0, 1, ..., L
```

where `aᵢ` are random polynomials and `eᵢ` are small noise terms. To extract `sk` from this, you would need to solve Ring-LWE: given `(aᵢ, aᵢ·sk + eᵢ + ...)`, find `sk`. This is the same hard lattice problem that protects the ciphertexts themselves.

In other words: **the evaluation keys are themselves encrypted forms of the secret key's powers**, encrypted under the same Ring-LWE assumption. Breaking the evaluation key = breaking the encryption scheme itself.

### 5. Which Key Is Used Where — Operation by Operation

#### Enrollment (client-side)

```cpp
// Client generates keys ONCE
auto ctx = FHEContext::create();
ctx->generate_keys();

// Client encrypts iris template using PUBLIC key
auto enc_tmpl = EncryptedTemplate::encrypt(*ctx, iris_code);
//                                         ↑ uses ctx.public_key()

// Client sends to server:
//   - enc_tmpl (2 ciphertexts: code + mask)
//   - public_key (so server can also encrypt if needed)
//   - eval keys (so server can compute on ciphertexts)
// Client keeps:
//   - secret_key (NEVER transmitted)
```

#### Server-side matching (NO secret key needed)

```cpp
// Server loads stored gallery and received probe (both encrypted)
// Server calls match_encrypted — uses ONLY evaluation keys internally

auto result = EncryptedMatcher::match_encrypted(ctx, probe, gallery);
//  ↑ Inside this function:
//    EvalSub  → no special key needed (addition/subtraction is "free" in BFV)
//    EvalMult → uses EvalMult key (relinearization) — stored in crypto context
//    EvalSum  → uses EvalRotate + EvalSum keys — stored in crypto context
//
//  The secret key is NEVER accessed. ctx.secret_key() is never called.
```

#### Client-side decryption (SECRET key required)

```cpp
// Only the client can do this — requires the secret key
auto hd = EncryptedMatcher::decrypt_result(ctx, result);
//  ↑ Inside this function:
//    ctx.context()->Decrypt(ctx.secret_key(), result.ct_numerator, &pt_num);
//    ctx.context()->Decrypt(ctx.secret_key(), result.ct_denominator, &pt_den);
//                           ↑ SECRET KEY — only the client has this
```

### 6. Key Lifecycle

```
TIME →
─────────────────────────────────────────────────────────────────────

ENROLLMENT (once per user)
  Client: generate_keys()
          ├── public_key    ──────────────────→ Server (store)
          ├── eval_keys     ──────────────────→ Server (store)
          ├── secret_key    → local secure storage (HSM, Keychain, TEE)
          └── encrypt(iris) ──────────────────→ Server (store ciphertext)

VERIFICATION (each time user authenticates)
  Client: encrypt(probe)   ──────────────────→ Server
  Server: match_encrypted(probe, gallery) ───→ encrypted_result
  Server: send encrypted_result ─────────────→ Client
  Client: decrypt_result(encrypted_result)   → HD score → match/no-match

RE-ENROLLMENT (key rotation — periodic security measure)
  Client: generate_keys()  → NEW key pair
          re_encrypt(old_ctx, new_ctx, old_ciphertext) → new ciphertext
          ├── Decrypt with OLD secret key
          └── Re-encrypt with NEW public key
          Send new ciphertext + new eval keys → Server
          Server: replace stored keys + ciphertext
          Client: securely delete OLD secret key

DELETE (user de-enrollment)
  Server: delete ciphertext files + eval keys
  Client: delete secret key
  → No recovery possible from any backup — ciphertexts without secret key
     are computationally indistinguishable from random data
```

### 7. Re-encryption: Key Rotation Without Server Trust

Our implementation includes `re_encrypt()` for key rotation:

```cpp
// From encrypted_template.cpp
Result<EncryptedTemplate> EncryptedTemplate::re_encrypt(
    const FHEContext& old_ctx,    // has OLD secret key (can decrypt)
    const FHEContext& new_ctx,    // has NEW key pair (will re-encrypt)
    const EncryptedTemplate& old_encrypted) {

    // Step 1: Decrypt with old secret key (client-side only)
    auto plaintext = old_encrypted.decrypt(old_ctx);

    // Step 2: Re-encrypt with new public key
    return encrypt(new_ctx, *plaintext);
}
```

Key points about re-encryption:
- **Happens entirely on the client** — the plaintext iris code is briefly in memory during the transition
- **Server never sees plaintext** — it only receives the new ciphertext
- **Old ciphertext becomes useless** — encrypted under the old public key, which no one will use again
- **Enables periodic key rotation** — limit exposure if a secret key is ever compromised

### 8. What the Server's `FHEContext` Contains vs. the Client's

| Component | Client's `FHEContext` | Server's `FHEContext` |
|-----------|----------------------|----------------------|
| Crypto context (parameters) | Yes | Yes (same params) |
| Public key | Yes | Yes |
| Secret key | **Yes** | **No** |
| EvalMult key | Yes (generated it) | Yes (received from client) |
| EvalSum key | Yes (generated it) | Yes (received from client) |
| EvalRotate keys | Yes (generated it) | Yes (received from client) |
| Can encrypt? | Yes | Yes (has public key) |
| Can decrypt? | **Yes** | **No** |
| Can compute on ciphertexts? | Yes | Yes |

In our current implementation, the `FHEContext` class holds all keys in one object. In a production deployment, you would split this:

```
ClientContext:  secret_key + public_key + eval_keys  (full access)
ServerContext:  public_key + eval_keys only          (compute-only access)
```

OpenFHE supports this via `CryptoContext::SerializeEvalMultKey()`, `SerializeEvalSumKey()`, and `SerializeEvalAutomorphismKey()` — serializing only the evaluation keys for server transport without the secret key.

### 9. Threat Model Summary

| Threat | Protection |
|--------|-----------|
| Server database breach | Ciphertexts without secret key → random data |
| Server memory dump during matching | Only polynomial coefficients visible — no plaintext at any point |
| Eval key theft | Cannot decrypt — only enables computation on ciphertexts attacker already has |
| Public key theft | Harmless — public by design, can only encrypt (not decrypt) |
| Secret key theft (client compromise) | **Full compromise** — attacker can decrypt all templates encrypted under this key |
| Man-in-the-middle (intercept ciphertext) | Useless without secret key; cannot modify ciphertext without detection (CCA security with integrity checks) |
| Quantum computer | Ring-LWE is believed quantum-hard (post-quantum secure); no known quantum speedup |

### 10. Secret Key Protection Strategies

The secret key is the single point of failure. Protecting it requires:

1. **Hardware Security Module (HSM)**: Store the key in tamper-resistant hardware. Key never leaves the HSM; decryption happens inside it.

2. **Platform Keychain / TEE**: On mobile devices, use iOS Keychain / Android Keystore / ARM TrustZone to store the key in hardware-backed secure storage.

3. **Key Splitting (Shamir's Secret Sharing)**: Split the secret key into N shares, require K-of-N to reconstruct. No single device compromise reveals the key.

4. **Ephemeral Keys**: Generate a fresh key pair for each verification session. Re-encrypt stored templates under the new key. Limits exposure window to one session.

5. **Key Derivation from Biometric**: Derive the secret key from a secondary biometric (e.g., fingerprint) via a key derivation function. The key exists only when the user's finger is on the sensor — no persistent storage needed.

The choice depends on the deployment model: a smartphone-based system might use the platform Keychain, while a national ID system would use HSMs with key splitting.

---

## Q: What is the cross-language validation test and how do I reproduce it?

**Date**: 2026-03-02

### What It Is

Cross-language validation proves that the C++ iris recognition pipeline produces **identical results** to the Python open-iris reference implementation. It runs both pipelines on the same images and compares every intermediate output and final result.

Three execution modes are compared:

| Mode | Pipeline | Matching | Purpose |
|------|----------|----------|---------|
| **Python** | Plaintext | Plaintext HD | Ground-truth reference |
| **C++ Plaintext** (`--no-encrypt`) | Plaintext | Plaintext HD (SIMD) | Apples-to-apples vs Python |
| **C++ Encrypted** (default) | Plaintext | Encrypted HD (BFV) | Production mode, FHE correctness |

**Sign-off is based on C++ Plaintext vs Python** (identical operations must produce identical results). Encrypted mode is informational + correctness verification (must agree with plaintext on match/no-match decisions).

### What Is Compared

| Dimension | Metric | Pass Threshold |
|-----------|--------|----------------|
| **Iris code bits** | `popcount(cpp XOR python)` | 0 (bit-exact) |
| **Hamming distance** | Pearson correlation across all pairs | r = 1.0, max discrepancy = 0.0 |
| **Match decisions** | Agreement at threshold 0.35 | 0 disagreements |
| **Per-node numerical** | Max absolute error per node | < 1e-4 (float), exact (int/bool) |
| **Biometric accuracy** | FMR / FNMR / EER | Delta = 0.0 vs Python |
| **Pipeline latency** | Mean speedup over Python | >= 3x |
| **Peak memory** | RSS | < 200MB, < 0.5x Python |
| **Encrypted correctness** | Encrypted HD vs Plaintext HD | Max diff < 1e-6 |

### Current Status

| Component | Status |
|-----------|--------|
| NPY reader/writer (C++) | Done — `tests/comparison/npy_reader.hpp`, `npy_writer.hpp`, `template_npy_utils.hpp` |
| NPY round-trip unit tests | Done — `test_npy_writer.cpp`, `test_template_npy.cpp` |
| Python fixture generator | **Not yet created** — `tools/generate_test_fixtures.py` |
| C++ comparison binary | **Not yet created** — `tests/comparison/iris_comparison.cpp` |
| Cross-language GTest suite | **Not yet created** — `tests/comparison/test_cross_language.cpp` |
| Comparison orchestrator | **Not yet created** — `tools/cross_language_comparison.py` |
| Dockerfile.comparison | **Not yet created** |
| CASIA1 dataset | Available — `data/Iris/CASIA1/` (108 subjects, 757 images) |
| ONNX segmentation model | **Not yet available** — required for full pipeline |

### Prerequisites

Before running cross-language validation, you need:

1. **CASIA1 dataset** — already at `data/Iris/CASIA1/` (757 JPEGs, 108 subjects)
2. **Python open-iris installed** — `cd open-iris && pip install -e .`
3. **ONNX segmentation model** — the `.onnx` file referenced in `pipeline.yaml` (not included in repo, must be obtained separately)
4. **C++ build with comparison target** — `cmake -DIRIS_BUILD_COMPARISON=ON -DIRIS_ENABLE_FHE=ON`

### How to Reproduce (Step-by-Step)

#### Step 0: Verify Dataset

```bash
# From project root
ls data/Iris/CASIA1/ | wc -l
# Expected: 108 (subject folders)

find data/Iris/CASIA1/ -name "*.jpg" | wc -l
# Expected: 757 (iris images)

# Sample structure: subject 001 has 7 images (left + right eye)
ls data/Iris/CASIA1/001/
# e.g.: 001_1_1.jpg  001_1_2.jpg  001_1_3.jpg  001_2_1.jpg ...
#        ^^^          ^^^
#        subject      L/R eye
```

#### Step 1: Set Up Python Environment

```bash
# Create isolated Python environment
python3 -m venv .venv
source .venv/bin/activate

# Install open-iris from the bundled submodule
cd open-iris
IRIS_ENV=SERVER pip install -e .
cd ..

# Verify installation
python3 -c "from iris import IRISPipeline; print('open-iris installed')"
```

#### Step 2: Generate Python Reference Fixtures

This runs every CASIA1 image through the Python pipeline and saves all 24 intermediate node outputs as `.npy` files plus scalar metadata as JSON.

```bash
# Create the fixture generator (not yet in repo — create tools/generate_test_fixtures.py)
python3 biometric/iris/tools/generate_test_fixtures.py \
    --images data/Iris/CASIA1/ \
    --output biometric/iris/tests/fixtures/python_reference/ \
    --env DEBUGGING_ENVIRONMENT
```

**What the fixture generator does** (pseudocode):

```python
import iris
import numpy as np
import json
from pathlib import Path

pipeline = iris.IRISPipeline(env=iris.IRISPipeline.DEBUGGING_ENVIRONMENT)

for image_path in sorted(Path("data/Iris/CASIA1").rglob("*.jpg")):
    result = pipeline(iris.IRImage(filepath=str(image_path)))

    out_dir = Path(f"fixtures/python_reference/{image_path.stem}")
    out_dir.mkdir(parents=True, exist_ok=True)

    # Save every intermediate node output as .npy
    np.save(out_dir / "segmentation_predictions.npy", result["segmentation"].predictions)
    np.save(out_dir / "geometry_mask_pupil.npy", result["vectorization"].pupil_array)
    np.save(out_dir / "geometry_mask_iris.npy", result["vectorization"].iris_array)
    np.save(out_dir / "normalized_image.npy", result["normalization"].normalized_image)
    np.save(out_dir / "normalized_mask.npy", result["normalization"].normalized_mask)
    np.save(out_dir / "iris_response_0_real.npy", result["iris_response"].iris_responses[0].real)
    np.save(out_dir / "iris_response_0_imag.npy", result["iris_response"].iris_responses[0].imag)
    np.save(out_dir / "iris_code.npy", result["encoder"].iris_codes)
    np.save(out_dir / "mask_code.npy", result["encoder"].mask_codes)

    # Save scalar metadata as JSON
    metadata = {
        "eye_centers": {
            "pupil": result["eye_center_estimation"].pupil_center,
            "iris": result["eye_center_estimation"].iris_center
        },
        "offgaze": result["offgaze_estimation"].score,
        "occlusion": result["occlusion_calculator"].score,
    }
    json.dump(metadata, open(out_dir / "metadata.json", "w"), indent=2)
```

**Expected output structure**:
```
biometric/iris/tests/fixtures/python_reference/
├── 001_1_1/
│   ├── segmentation_predictions.npy
│   ├── normalized_image.npy
│   ├── iris_code.npy
│   ├── mask_code.npy
│   ├── metadata.json
│   └── ...
├── 001_1_2/
│   └── ...
└── ... (757 image directories)
```

#### Step 3: Build C++ Comparison Binary

```bash
cd biometric/iris

# Inside Docker (recommended)
docker compose build test

# Or build comparison target specifically:
# cmake -B build -G Ninja \
#     -DCMAKE_BUILD_TYPE=Release \
#     -DIRIS_ENABLE_TESTS=ON \
#     -DIRIS_ENABLE_FHE=ON \
#     -DIRIS_BUILD_COMPARISON=ON \
#     -DONNXRUNTIME_ROOT=/opt/onnxruntime
# cmake --build build --target iris_comparison test_cross_language
```

#### Step 4: Run C++ in Plaintext Mode

```bash
# Process same images through C++ pipeline, save outputs as .npy
./build/tests/comparison/iris_comparison \
    --no-encrypt \
    --images ../../data/Iris/CASIA1/ \
    --output tests/fixtures/cpp_plaintext/ \
    --format npy
```

#### Step 5: Run C++ in Encrypted Mode

```bash
# Same pipeline, but matching uses FHE
./build/tests/comparison/iris_comparison \
    --images ../../data/Iris/CASIA1/ \
    --output tests/fixtures/cpp_encrypted/ \
    --format npy
```

#### Step 6: Run Comparison

```bash
# Orchestrator compares all three modes and generates report
python3 tools/cross_language_comparison.py \
    --python-fixtures tests/fixtures/python_reference/ \
    --cpp-plaintext-fixtures tests/fixtures/cpp_plaintext/ \
    --cpp-encrypted-fixtures tests/fixtures/cpp_encrypted/ \
    --output comparison_report.json

# Check pass/fail
python3 -c "
import json, sys
r = json.load(open('comparison_report.json'))
print(f'Sign-off pass: {r[\"signoff_pass\"]}')
print(f'Encryption correctness: {r[\"encryption_correctness_pass\"]}')
print(f'Overall pass: {r[\"overall_pass\"]}')
if r['failures']:
    print('FAILURES:')
    for f in r['failures']:
        print(f'  - {f}')
    sys.exit(1)
"
```

#### Step 7: Run GTest Cross-Language Suite

```bash
# C++ test that loads Python .npy fixtures and asserts numerical agreement
cd build
ctest --test-dir . -R CrossLanguage --output-on-failure
```

### What the Comparison Report Contains

```json
{
  "signoff_pass": true,
  "encryption_correctness_pass": true,
  "overall_pass": true,

  "signoff": {
    "accuracy": {
      "iris_code_bit_agreement": {"rate": 1.0, "pass": true},
      "hamming_distance_correlation": {"pearson_r": 1.0, "max_discrepancy": 0.0, "pass": true},
      "decision_agreement": {"threshold": 0.35, "disagree": 0, "pass": true},
      "biometric": {"eer_delta": 0.0, "pass": true}
    },
    "performance": {
      "pipeline_latency": {"speedup": 4.72, "pass": true},
      "memory_peak_rss_mb": {"cpp_plaintext": 160, "pass": true}
    }
  },

  "encryption_report": {
    "correctness": {
      "decision_agreement_vs_plaintext": {"disagree": 0, "pass": true},
      "hd_value_agreement_vs_plaintext": {"max_abs_diff": 0.0, "pass": true}
    }
  }
}
```

### What's Blocking This Checklist Item

| Blocker | Why It's Needed | Resolution |
|---------|-----------------|------------|
| **ONNX segmentation model** | First node in pipeline — without it, no iris codes can be produced | Obtain the `.onnx` model file from Worldcoin/open-iris model card or train one |
| **`generate_test_fixtures.py`** | Generates the Python ground-truth `.npy` files | Must be written (pseudocode above) |
| **`iris_comparison.cpp`** | C++ binary that processes images and outputs `.npy` for comparison | Must be written |
| **`cross_language_comparison.py`** | Orchestrator that compares outputs across 3 modes | Must be written |
| **`test_cross_language.cpp`** | GTest suite loading `.npy` fixtures and asserting agreement | Must be written |
| **`Dockerfile.comparison`** | Container with both Python open-iris + C++ binary | Must be written |

### Quick Validation (Without Full Framework)

Even without the full comparison framework, you can do a partial validation:

```bash
# 1. Generate a Python iris code for one image
python3 -c "
import iris, numpy as np
pipeline = iris.IRISPipeline()
result = pipeline(iris.IRImage(filepath='data/Iris/CASIA1/001/001_1_1.jpg'))
np.save('/tmp/python_iris_code.npy', result.template.iris_codes)
np.save('/tmp/python_mask_code.npy', result.template.mask_codes)
print(f'Iris code shape: {result.template.iris_codes.shape}')
print(f'HD with self: {iris.HammingDistanceCalculator()(result.template, result.template)}')
"

# 2. Load the .npy in C++ and compare
# (use npy_reader.hpp to load, compare bit-by-bit)

# 3. Run existing unit tests that verify internal consistency
cd biometric/iris && make test
# 442 tests passing — verifies all algorithms work correctly in isolation
```

### Tolerance Table (Per Pipeline Node)

| Node | Output Type | Comparison Method | Tolerance |
|------|------------|-------------------|-----------|
| Segmentation | float32 prediction map | Max absolute error | < 1e-4 |
| Binarization | uint8 class map | Exact match | 0 |
| Vectorization | float64 contour points | Hausdorff distance | < 1.0 pixel |
| Geometry Refinement | float64 refined points | Hausdorff distance | < 0.5 pixel |
| Geometry Estimation | float64 ellipse params | Relative error | < 0.1% |
| Eye Center Estimation | float64 (x, y) | Euclidean distance | < 0.5 pixel |
| Offgaze Estimation | float64 score | Absolute error | < 1e-6 |
| Occlusion Calculator | float64 score | Absolute error | < 1e-6 |
| Normalization | float64 normalized image | PSNR | > 60 dB |
| Iris Response (Gabor) | float64 filter response | Max absolute error | < 1e-5 |
| Encoder | uint8 iris/mask code | Bit-exact match | 0 |
| Hamming Distance | float64 | Exact match | 0.0 |
| Match Decision | bool | Exact match | 0 disagreements |

---

## Q: What is the target platform? Why Linux ARM only?

**Date**: 2026-03-02

### Decision

**Target platform**: Linux aarch64 (ARM64) only.

- **Not targeted**: x86_64, macOS native, Windows.
- **Development environment**: macOS Apple Silicon (M4) — used for editing and running Docker builds only. No native macOS builds are produced.

### Rationale

1. **Production hardware is ARM**: The deployment target is Linux on ARM processors. There is no need to support x86_64 in production.
2. **Docker on Apple Silicon runs ARM natively**: On macOS M4, Docker runs `linux/arm64` containers without emulation (no Rosetta overhead). This means the dev environment and production target use the same architecture.
3. **Simplification**: Supporting multiple architectures (x86_64 + ARM + macOS) adds build complexity, CI cost, and testing surface area with no production benefit.
4. **SIMD**: NEON is the primary SIMD target. AVX2/AVX-512 code is retained in `matcher_simd.hpp` for compile-time compatibility but is not tested or optimized as a target.

### What This Means in Practice

| Activity | Where | Architecture |
|----------|-------|-------------|
| Code editing | macOS M4 (local) | arm64 |
| Building | Docker (Ubuntu 24.04) | aarch64 |
| Testing (`make test`) | Docker (Ubuntu 24.04) | aarch64 |
| Benchmarking | Docker (Ubuntu 24.04) | aarch64 |
| Production deployment | Linux server | aarch64 |

### Build Command

```bash
cd biometric/iris && docker compose build --no-cache test
# Builds and runs all 445 tests on Linux aarch64
```

### Files Updated

- `MASTER_PLAN.md §14` — platform table, SIMD strategy, build presets, dev notes
- `CHECKLIST.md` — removed macOS/x86_64 gate items, updated SIMD audit
- `TESTING.md` — preset table, platform note
- `CMakePresets.json` — removed `macos-arm64` preset
- `CMakeLists.txt` — annotated x86 SIMD section as retained for compatibility only
- `matcher_simd.hpp` — NEON is primary, AVX2/AVX-512 compile-time fallbacks retained

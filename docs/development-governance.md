Below is a **single canonical document** you can commit to the repo and reuse across ChatGPT sessions.
It encodes the policy, document roles, stability model, and AI operating rules in a way that is durable and unambiguous.

Save this as:

```
docs/development-governance.md
```

---

# Bash Diamonds — Development Governance

This document defines how specifications, documentation, examples, and AI-assisted development are to be handled across iterations. It is authoritative for repository workflow and documentation structure.

---

## 1. Document Roles and Precedence

### 1.1 Normative Contract

`docs/project-spec.md` is the **only normative suite-wide contract**.

It defines:

* Suite-wide behavioral invariants
* Exit code semantics
* Option parsing rules
* Stdin/file handling model
* Diagnostic behavior
* Stability guarantees
* Builtin inventory and status

If any other document conflicts with `project-spec.md`, `project-spec.md` prevails.

---

### 1.2 Builtin Documentation

Each builtin has its own documentation file.

Builtin documentation:

* Is normative for that builtin.
* May extend suite invariants.
* Must not weaken or contradict suite invariants.
* Must fully define v1.0 behavior when marked stable.

---

### 1.3 Handbook (User Guide)

`docs/diamond-suite.md` is the **user-facing handbook**.

It is:

* Non-normative.
* A guide to composition and usage.
* Written for end users, not just developers.
* Copy/paste friendly.
* Executable without error on a typical POSIX system with bash.

The handbook must not define contract rules.

If handbook examples conflict with the contract, the handbook is incorrect.

---

## 2. Builtin Stability Model

The suite uses the following status classifications:

* **stable**
  Part of the v1.0 contract. Behavior and output formats are guaranteed within the constraints of the suite invariants.

* **preview**
  Shipped and usable but subject to change. Not yet part of the stability guarantee.

* **planned**
  Not implemented or placeholder only.

`docs/project-spec.md` maintains the authoritative builtin inventory and their status.

The repository state (source code) does not define stability — the spec does.

---

## 3. Handbook Policy (User-Facing Requirements)

The handbook exists for users.

The following rules apply:

1. `diamond-suite.md` is user-facing and written for copy/paste use.
2. Examples must run on a typical POSIX-like system using bash.
3. Examples must execute without error.
4. CI validates runnability and claimed outputs only.
5. Handbook examples are not benchmarks and must not imply production representativeness.
6. Realistic data examples should use fixtures stored under `examples/` where appropriate.

The handbook must avoid internal build paths and unnecessary harness complexity.

---

## 4. Documentation Example Validation Policy

Documentation examples may be validated in CI.

CI validation guarantees:

* Commands execute successfully.
* Exit codes match what is declared.
* When output is explicitly asserted, it matches.

CI validation does **not** imply:

* Production performance validation.
* Exhaustive edge-case coverage.
* Workload representativeness.

Documentation validation exists solely to prevent drift and copy/paste failure.

---

## 5. Suite Invariants Governance

Suite invariants live only in `project-spec.md`.

Changes to suite invariants are considered contract-level modifications and must:

* Be explicitly updated in `project-spec.md`.
* Be reflected in relevant builtin documentation.
* Be accompanied by test updates.
* Be treated as potentially breaking changes.

Suite invariants must remain consistent across all builtins.

---

## 6. Merge Discipline (Feature Branch Policy)

On every merge from `feature/<name>` into `main`, the following must be evaluated:

1. Does this change alter behavior defined in `project-spec.md`?
2. Does it alter any builtin contract?
3. Does it affect handbook examples?
4. Does it change stability status?

If yes:

* Update `project-spec.md`.
* Update builtin documentation.
* Update handbook examples if needed.
* Update tests accordingly.

No unrelated refactors should be introduced during feature merges.

---

## 7. AI-Assisted Development Protocol

This section exists to ensure consistency across ChatGPT sessions.

When starting a new AI session:

* Treat `docs/project-spec.md` as the only normative suite contract.
* Treat `docs/diamond-suite.md` as a user handbook only.
* Maintain the stability model (stable / preview / planned).
* Do not introduce behavior inconsistent with suite invariants.
* Do not weaken defined contracts.
* Keep documentation and implementation synchronized.
* Avoid broad refactors outside the current feature goal.
* Preserve exit code semantics and newline determinism.

AI-generated changes must respect this governance document.

If ambiguity arises, prefer preserving the suite contract over expanding behavior.

---

## 8. Production Reality Acknowledgment

The test suite validates correctness and invariants.
It does not claim to represent production workloads.

Users are responsible for evaluating suitability for their own workloads.

The suite guarantees behavioral correctness, not performance characteristics.

---

# End of Governance Document

---

## How to Use This Across ChatGPT Sessions

When beginning a new session, paste this instruction block:

> Follow `docs/development-governance.md` as binding project rules.
> `project-spec.md` is the normative contract.
> `diamond-suite.md` is the user handbook.
> Maintain stability classifications.
> Do not introduce behavior inconsistent with suite invariants.
> Keep docs and implementation synchronized.

This ensures continuity even if session memory resets.

---

If you want, the next step can be:

* A clean restructuring plan for `project-spec.md` reflecting this governance,
  or
* A minimal patch outline to convert the current scaffold to this model.

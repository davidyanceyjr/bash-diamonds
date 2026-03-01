Good. Then the handbook should read like something a shell user actually wants to read — not a spec, not a whitepaper.

It should feel practical. Concrete. Opinionated about clarity. Focused on replacing messy one-liners with small, readable pipelines.

Below is a revised draft of `docs/diamond-suite.md` written as a real how-to guide.

---

# Bash Diamonds

## A Practical Guide to Cleaner Shell Pipelines

Bash Diamonds is a small set of shell builtins for working with text.

It exists for one reason:

To make common shell pipelines simpler, clearer, and more predictable.

If you’ve ever written something like:

```bash
cat file | awk '{print $2}' | grep foo | wc -l
```

and then had to come back later and remember what it does — Bash Diamonds is for you.

---

## Why This Exists

Tools like `sed`, `grep`, `awk`, and `find` are powerful.

They are also:

* Dense
* Syntax-heavy
* Hard to compose cleanly
* Inconsistent in exit behavior
* Easy to overuse

Bash Diamonds takes a different approach:

* One tool, one job.
* Consistent behavior.
* Simple, readable pipelines.
* Built directly into Bash.

The goal is not to replace Unix tools.

The goal is to make everyday text manipulation easier to read and reason about.

---

## The Basic Idea

Each tool in the suite does one small transformation.

You compose them.

Instead of writing:

```bash
awk '{print $1}' file | grep foo | wc -l
```

You write:

```bash
fields -f 1 file | match -m foo | count
```

It reads like a story:

* Extract the first field.
* Keep lines that match.
* Count them.

That’s it.

---

## How These Tools Fit Into Your Workflow

You already use pipelines.

Bash Diamonds just makes them cleaner.

### Extract → Filter → Count

```bash
printf "a 1\nb 2\nc 3\nb 4\n" \
  | fields -f 1 \
  | match -m b \
  | count
```

This:

* Extracts the first column
* Keeps only `b`
* Counts occurrences

No awk blocks.
No regular expression clutter.
No trailing whitespace surprises.

---

### Clean Up Input

You get messy input. It happens.

```bash
printf "  apple\nbanana  \n pear \n" | trim
```

Trimming is explicit. No side effects.

---

### Limit Output

Instead of:

```bash
head -n 5
```

You can stay in the same vocabulary:

```bash
take -n 5
```

Same idea. Same pipeline style.

---

### Align Columns for Humans

You often just want readable output:

```bash
printf "a bb c\naaa b ccc\n" | table
```

It aligns columns without rewriting your data.

---

## Builtins, Not External Commands

These tools are Bash builtins.

Once enabled, they behave like native commands.

```bash
enable -f build/filter.debug.so filter
```

No subshell cost.
No extra processes.
No external dependencies.

---

## Consistent Exit Behavior

Every tool behaves the same way:

* Exit `0` when it produces output
* Exit `1` when it runs successfully but produces nothing
* Exit `2` when something goes wrong

That makes shell scripting predictable:

```bash
if fields -f 1 file | match -m foo; then
    echo "Found it"
else
    echo "Not found"
fi
```

You don’t need to guess what the exit code means.

---

## Composing Tools

The strength of Bash Diamonds is composition.

### Example: Count users with shell `/bin/bash`

```bash
cat /etc/passwd \
  | fields -d: -f 7 \
  | match -m /bin/bash \
  | count
```

Readable. Direct. Intentional.

---

### Example: First 3 unique-looking values

```bash
printf "a\na\nb\nc\nc\n" \
  | take -n 3
```

Small transformations stack cleanly.

---

## Philosophy in Practice

Bash Diamonds favors:

* Clarity over cleverness
* Small steps over complex expressions
* Deterministic behavior over magic
* Explicit transformations over implicit ones

If a pipeline feels like a puzzle, it probably needs fewer tricks and more structure.

---

## When to Use These Tools

Use Bash Diamonds when:

* You’re building readable shell scripts.
* You want consistent exit semantics.
* You’re composing multiple text transformations.
* You don’t want to embed awk programs inside pipelines.

Don’t use it when:

* You need full programming-language features.
* You need heavy text parsing or advanced regex features.
* Performance requires specialized tools.

These are diamonds — small, sharp tools.

---

## Stability

Some tools are stable and part of the v1.0 contract.

Others are evolving.

For authoritative guarantees, see `project-spec.md`.

---

## A Better Way to Think About Shell Pipelines

Instead of writing one dense command that does everything:

Break it up.

Name the transformations.

Compose small tools.

Make your future self happy.

---

# End of Handbook Draft

---

This version:

* Reads like a guide, not a spec.
* Explains motivation.
* Frames tools as workflow improvements.
* Encourages composition imagination.
* Avoids contract language.
* Keeps examples simple and human.

If you'd like, next we can:

* Add a “Common messy pipeline → Diamond rewrite” comparison section
  or
* Add a short “From awk/grep to Diamonds” translation guide for users transitioning from traditional Unix tools.

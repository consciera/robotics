# Consciera · Robotics

The working record behind the robotics series on
[Consciera](https://youtube.com/@Consciera).

The videos carry one mechanism each, explained so that someone without a
technical background can follow it. This repository carries everything the
videos leave out: what was attempted, what failed, the configuration, the
commands, the dead ends, and the predictions made before each session.

---

## The rule

**Every session is logged, and the log is published whatever it contains.**

A session where nothing worked gets written up at the same length as one where
something did. Sessions are not omitted for being unflattering, inconclusive,
or boring.

This is the only promise the repository makes, and it is the reason to trust
anything else in it.

---

## Predictions come first

Before each session, a prediction is written down and said out loud on camera
— then scored in the same video, right or wrong.

Predictions are deliberately phrased so a viewer can score them without help:
*it will take more than three attempts*, not *the joint will reach 47 degrees
before the torque limit trips*. If the audience needs us to tell them whether
we were right, the prediction was too technical to be worth making.

**Wrong predictions stay in.** A prediction that only appears when it lands is
not a prediction.

The accumulating interest is not the hit rate — it is **the shape of the
errors.** If we turn out to be consistently wrong in the same direction about
how long the unglamorous parts take, that is the finding.

---

## Naming

Hardware keeps the name its manufacturer gave it. The integration takes a
`CAS-` prefix.

```
CAS-R1      the Unitree R1, running our control stack
```

**The prefix is earned, not assigned.** It is `R1` until code we wrote moves
it. A failed integration keeps its number; nothing is recycled.

---

## Layout

```
/cas-r1/
    log/            session logs, newest last, append-only
    predictions/    written before each session, never edited after
    src/            the code, as it actually ran
    notes/          hardware findings, state transitions, gotchas
```

A second integration on different hardware gets its own top-level directory.
One repository, because the log should not be split when the machine changes.

---

## What is not here

**Vendor documentation.** Where the manufacturer's docs are correct, we link
to them. Where they are wrong, that goes in `notes/` with what actually
happened.

**Anything identifying.** No serial numbers, network addresses, hostnames, or
location details. This is a working log, not a configuration you can clone.

**Polish.** The logs are written during or immediately after a session and are
not tidied later. Edits to a published log are additive and dated.

---

## The videos

[youtube.com/@Consciera](https://youtube.com/@Consciera) ·
[consciera.ai](https://consciera.ai)

The series examines what humanoid robots actually do, using one machine, in
one room, with the failures left in.

# Survival Endgame — Stage 10

Stage 10 completes the replayability layer of Survival. It does not add permanent
progression and it does not implement Expedition gameplay.

## Run setup

The flow is `Character Selection -> Survival Setup -> Playing`. Quick Start always
uses Normal, Ascension 0, Standard challenge, no mutators and non-Endless rules.
Custom Start resolves one `SurvivalRunConfig` containing difficulty, Ascension,
Endless, challenge and a compact mutator mask. A challenge replaces the editable
fields with its predefined rules. Restart reuses the resolved config and character;
leaving to the menu clears runtime systems. Settings persistence remains separate.

## Difficulty and Ascension

- Normal: baseline values and score x1.00.
- Hard: enemy HP x1.20, damage x1.15, spawn interval x0.90, elite chance x1.30,
  miniboss HP x1.18, boss HP x1.22, boss damage x1.12, score x1.25.
- Nightmare: enemy HP x1.42, damage x1.30, spawn interval x0.82, pressure x1.10,
  elite chance x1.70, miniboss HP x1.38, boss HP x1.48, boss damage x1.25,
  health-drop chance x0.80, score x1.60.

Ascension ranges from 0 to 10. Each level adds 2.5% enemy HP, 1.8% enemy damage,
6% elite chance, 3.5% miniboss HP, 3% boss HP and 5% score, while reducing spawn
interval by 1.2% with a floor. Milestones at A4, A6 and A8 respectively increase
event frequency, normal pressure and special-wave frequency.

## Mutators

Up to three are selectable. Hyper Horde and Titanic are incompatible.

- Glass Cannon: player damage x1.40, max HP x0.60, score x1.18.
- Hyper Horde: interval x0.62, pressure x1.35, enemy HP x0.68, score x1.16.
- Titanic: enemy HP x2.15, interval x1.40, pressure x0.62, score x1.18.
- Elite Infestation: elite chance x3.20, pressure x0.84, score x1.28.
- Blood Rush: enemy speed x1.12, player speed x1.15, score x1.08.
- Scarce Recovery: health-drop chance x0.30 with a non-zero floor, score x1.14.
- Eventful: event interval x0.58, score x1.12.
- Chaotic Waves: special-wave interval x0.58, score x1.12.

## Challenges

- The Swarm: Hard A2, Hyper Horde + Scarce Recovery + Chaotic Waves, forced swarm composition.
- Titans: Hard A3, Titanic, durable composition.
- Night of Elites: Nightmare A2, Elite Infestation.
- Blood Moon: Hard A4, Eventful + Scarce Recovery, prefers Blood Moon events.
- Boss Hunter: Nightmare A3, Chaotic Waves, stronger bosses/minibosses and extra miniboss scheduling.
- Fragile Power: Nightmare A1, Glass Cannon + Blood Rush.

Challenges are Survival presets, not separate modes. They are locked while selected
and disable Endless. There is no completion reward or persistent unlock in Stage 10.

## Endless

The 15-minute Ascended boss gives a chest and the run continues. From 20:00 onward,
a boss is scheduled every five minutes, rotating Flame Wyrm, Void Herald and Void
Herald Ascended. A cycle is 15 minutes. Cycles progressively raise enemy HP/damage,
elite chance and boss HP while reducing spawn interval. Scaling uses cycle 8 as a
soft cap; speed, pressure, recovery, interval and score have explicit safe bounds.

Endless has no Victory condition. It ends only with player death or leaving the run.
Results report full time, completed cycles and the score breakdown. Existing pools,
enemy caps and boss encounter gating remain authoritative.

## Score

Base score is:

`eligible kills * 10 + elites * 75 + minibosses * 300 + bosses * 1200 + floor(seconds) * 2`

The base is multiplied by difficulty, Ascension and mutator/challenge modifiers,
then rounded once. Necromancer summons are excluded from eligible kills. Pause does
not update elapsed time or score because only `Playing` advances simulation.

## Debug and validation

With the F3 debug gate enabled, setup accepts `D` (difficulty), `A` (Ascension),
`L` (Endless), `U` (cycle/toggle mutator) and `J` (challenge). During gameplay,
`F10` advances one minute and `Y` advances five minutes. Existing boss/event/wave
tools remain available. The overlay shows the resolved config, cycle, score
multiplier and live score.

Automated tests cover defaults, all tiers, Ascension 10, every mutator, three-mutator
selection, incompatibility, all challenges, actual boss scaling, summon exclusion,
score calculation, Endless final-boss continuation, rotating bosses and clean restart.

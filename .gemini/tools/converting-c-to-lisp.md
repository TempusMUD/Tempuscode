# Converting C to Lisp

This skill provides guidance for working with Common Lisp code in the Tempus MUD codebase.

When converting C code to Common Lisp, follow these guidelines:

### 1. Naming Conventions

- C `snake_case` → Lisp `kebab-case`
- C `UPPER_CASE` constants → Lisp `+kebab-case+` parameters
- C `GET_FIELD(obj)` macros → Lisp `(field-of obj)` accessors
- C `IS_FLAG(obj)` predicates → Lisp `(is-flag obj)` or `(flag-p obj)`
- C `NPC_FLAGGED(ch, flag)` → Lisp `(mob-flagged ch flag)` for NPCs
- C `NPC2_FLAGGED(ch, flag)` → Lisp `(mob2-flagged ch flag)` for extended NPC flags

### 2. Spell Definitions

- Use `define-spell` macro for standard spells with target parameter
- The macro provides implicit parameters: `caster`, `target`, `level`, `saved`
- For spells that work on objects (like animate-dead), `target` can be either a creature or object
- Check type with `(typep target 'creature)` to distinguish between creature and object targets
- Spells should only be defined using `defun` in very specific circumstances.

### 3. Common Pitfalls

- `is-fighting` doesn't exist - use `(fighting-of ch)` and check if non-nil
- `ok-to-attack` is in C but `can-attack` is the Lisp version
- `free-creature` in C is `extract-creature` in Lisp, which takes 2 args: `(extract-creature ch nil)`
- `isname` in C is `is-alias-of` in Lisp
- Flag checking: use `mob-flagged` not `npc-flagged`, `mob2-flagged` not `npc2-flagged`
- String formatting: Lisp uses `~a` for strings and `~d` for numbers in format/send-to-char
- Setting bitflags: use `(setf (aff-flags-of obj) (logior ...))` not `(setf (aff-flagged obj flag) t)`
- Removing bitflags: use `(logandc2 (aff-flags-of obj) flag)` not
  setting to nil
- the `floor` function has a divisor as an optional second argument.

### 4. File Organization

- Group related spells in files like `spells-summoning.lisp`, `spells-teleportation.lisp`
- Keep helper functions in the same file or appropriate utility files
- Test Lisp files by loading them: `(load "path/to/file.lisp")`
- Make sure to add new files to the asdf project file.

### 5. Testing Lisp Code

- Use lisp tool to interact with a running lisp image.  If there is no
  lisp tool, stop and warn the user.
- Look for undefined functions/variables in compiler warnings
- Simple functions can be simply called to test them.

### 6. Common Helper Functions

- `(is-npc ch)`, `(is-pc ch)` - check creature type
- `(is-dead ch)` - check if creature is dead
- `(can-see-creature ch target)` - visibility check
- `(can-attack ch target)` - attack validation
- `(perform-hit ch target)` - initiate combat
- `(start-hunting ch target)`, `(stop-hunting ch)` - hunting behavior
- `(find-first-step src dest mode)` - pathfinding (mode is nil for standard tracking)
- `(find-distance start dest)` - calculate room distance
- `(emit-voice ch target situation)` - NPC voice/emotes (situations are keywords like `:hunt-found`)

### 7. Important Constants

- Tracking modes: pass `nil` for standard tracking (not `+track-std+`)
- Mob flags: `+mob-spirit-tracker+`, `+mob2-silent-hunter+`
- Position: `+pos-standing+`, `+pos-sleeping+`, etc.
- Affection flags: `+aff-invisible+`, `+aff2-haste+`, etc.
- Spell constants: `+spell-chill-touch+`, `+spell-local-teleport+`, etc.

### 8. Special Procedures (Mobile/Object/Room Specials)

#### Defining Specials

- Use `define-special` macro to create special procedures
- Syntax: `(define-special name (trigger self ch command vars) (flags) body...)`
- Parameters:
  - `trigger` - the event type (e.g., `'tick`, `'command`, `'death`, `'enter`)
  - `self` - the mob/object/room with the special
  - `ch` - the creature triggering the event (may be nil for tick)
  - `command` - command info for command triggers
  - `vars` - additional variables depending on trigger type
- Flags: `+spec-mob+`, `+spec-obj+`, `+spec-rm+` (can combine with `logior`)
- Returns: `t` if special handled the event, `nil` otherwise
- The macro automatically registers the special in `*special-funcs*` hash table

#### Special Procedure Patterns

**Tick-based specials:**
```lisp
(define-special my-special (trigger self ch command vars) (+spec-mob+)
  (declare (ignore command vars ch))
  (when (eql trigger 'tick)
    ;; Periodic actions here
    ))
```

**Combat behavior:**
- Use `(fighting-of mob)` to check if in combat (returns list of opponents or nil)
- Use `(random-elt (fighting-of mob))` to get a random opponent (equivalent to C's `random_opponent()`)
- Use `(cast-spell caster target obj-target arg spell-num level)` to cast spells
  - For mob-to-creature spells: target is creature, obj-target is nil
  - For self-cast: target is self, obj-target is nil
  - level parameter is usually `(level-of mob)` or modified (e.g., `(+ (level-of mob) 10)`)

**Purging/Removing mobs:**
- Use `(purge-creature mob destroy-equipment)` to remove a mob
- Second argument: `t` to destroy equipment, `nil` to drop it

**Room flags check:**
- Use `(room-flagged room flag)` to check single flag
- Example: `(room-flagged (in-room-of mob) +room-nomagic+)`
- Can combine checks: `(or (room-flagged room +room-nomagic+) (room-flagged room +room-norecall+))`

**Hunting/Tracking:**
- Use `(hunting-of mob)` to get current hunt target (returns creature or nil)
- Use `(start-hunting mob target)` to begin hunting
- Use `(stop-hunting mob)` to stop hunting

#### Special File Organization

- Mobile specials in `lisp/src/specials/` directory
- Use descriptive filenames (e.g., `unholy-stalker.lisp`, `fido.lisp`)
- Start file with `(in-package #:tempus)`
- No need to manually register - `define-special` handles registration automatically
- Special name converted to string with underscores: `unholy-stalker` → "unholy_stalker"

## Project Structure

- Lisp code in `lisp/` directory (Common Lisp implementation of game logic)
- Spell implementations in `lisp/src/actions/spells*.lisp`
- Special procedures in `lisp/src/specials/`
- Utility functions in `lisp/src/utilities/`
- Constants in `lisp/src/compile/constants.lisp`

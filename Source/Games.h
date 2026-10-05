#pragma once
#include <juce_core/juce_core.h>
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>
#include "DspUtil.h"
#include "MelodyGen.h"
#include "Living.h"

// v0.45 GAME (EVOLVE ARCADE): the pure game state and physics (no drawing, no audio) - SOUND INVADERS, PIXEL DUEL, PINBALL,
// the CHEST (1-5 sounds, COMMON / RARE / EPIC / LEGENDARY) and the COLLECTION BOOK (250 recipe slots).
// Every hit is an event with a pitch in the key (pentatonic) and a velocity; the page plays it on the keys.
// Unlocks are deterministic per game / level / score: the same run always opens the same chest, so the book means something.
namespace kk::game
{
enum GameId { gInvaders, gDuel, gPinball, numGames };
inline const char* gameName (int g) { static const char* n[] { "SOUND INVADERS", "PIXEL DUEL", "PINBALL" }; return n[juce::jlimit (0, 2, g)]; }

// ---------------- the key: everything sounds in a pentatonic ----------------
struct Key
{
    int root = 9; bool minor = true;
    int scale() const { return minor ? kk::mel::scPentaMinor : kk::mel::scPentaMajor; }
    int pitch (int degree, float range = 0.5f) const { return kk::mel::degreeToPitch (degree, root, scale(), range); }
    bool inKey (int p) const { const auto& st = kk::mel::scaleSteps (scale()); return std::find (st.begin(), st.end(), ((p - root) % 12 + 12) % 12) != st.end(); }
};

// a game event: something to hear / flash.  pitch < 0 = silent
struct Ev { int kind = 0; float x = 0, y = 0; int pitch = -1; float vel = 0.8f; int ref = -1; };

// ================================================================ SOUND INVADERS ================================================================
// field 0..1 x 0..1 (y down).  move, shoot; every destroyed enemy is a note in the key; every 3rd wave = the 808 BOSS.
struct Invaders
{
    enum EvKind { evKill, evBossHit, evBossKill, evPlayerHit, evWaveClear, evGameOver, evShoot };
    struct Enemy { float x = 0, y = 0; int row = 0, col = 0; bool alive = true; };
    struct Shot { float x = 0, y = 0, vx = 0, vy = 0; bool enemy = false; };
    static constexpr float playerY = 0.9f, enemyW = 0.052f, enemyH = 0.042f, bossW = 0.2f, bossH = 0.12f;
    Key key; Rng rng;
    float px = 0.5f, t = 0, dir = 1, fireCd = 0, invuln = 0, comboT = 0, waveT = 0;
    int lives = 3, score = 0, wave = 1, combo = 0, rows = 4, kills = 0;
    std::vector<Enemy> enemies; std::vector<Shot> shots;
    bool boss = false, over = false, victory = false;
    float bossX = 0.5f, bossY = 0.17f, bossHp = 0, bossMax = 1, bossFire = 0, bossFlash = 0;

    int level() const { return (wave - 1) / 3 + 1; }
    bool bossWave() const { return wave % 3 == 0; }
    void start (uint32_t seed, Key k) { key = k; rng.seed (hash32 (seed * 31u + 7u)); px = 0.5f; lives = 3; score = 0; wave = 1; over = victory = false; shots.clear(); spawnWave(); }
    void spawnWave()
    {
        enemies.clear(); shots.clear(); dir = 1; combo = 0; waveT = 0;
        boss = bossWave();
        rows = boss ? 1 : std::min (5, 3 + (wave + 1) / 2);
        const int cols = boss ? 6 : 8;
        for (int r = 0; r < rows; ++r)
            for (int c = 0; c < cols; ++c)
                enemies.push_back ({ (boss ? 0.25f : 0.14f) + (float) c * (boss ? 0.1f : 0.08f), (boss ? 0.33f : 0.12f) + (float) r * 0.07f, r, c, true });
        if (boss) { bossMax = bossHp = 24.0f + 8.0f * (float) level(); bossX = 0.5f; bossY = 0.15f; bossFire = 1.0f; }
    }
    int alive() const { int n = 0; for (auto& e : enemies) n += e.alive; return n; }
    // the note of a kill: higher rows sing higher, a quick combo climbs the scale
    int killPitch (const Enemy& e) const { return key.pitch ((rows - 1 - e.row) * 2 + (combo % 5), 0.5f); }

    std::vector<Ev> step (float dt, bool left, bool right, bool fire)
    {
        std::vector<Ev> ev;
        if (over) return ev;
        t += dt; waveT += dt;
        fireCd = std::max (0.0f, fireCd - dt); invuln = std::max (0.0f, invuln - dt); bossFlash = std::max (0.0f, bossFlash - dt * 4);
        comboT -= dt; if (comboT <= 0) combo = 0;
        px = juce::jlimit (0.04f, 0.96f, px + ((right ? 1.0f : 0.0f) - (left ? 1.0f : 0.0f)) * 0.62f * dt);
        int mine = 0; for (auto& s : shots) mine += ! s.enemy;
        if (fire && fireCd <= 0 && mine < 2) { shots.push_back ({ px, playerY - 0.03f, 0, -1.45f, false }); fireCd = 0.26f; ev.push_back ({ evShoot, px, playerY }); }
        // the formation marches and drops
        const int n = alive(), total = std::max (1, (int) enemies.size());
        const float speed = 0.05f + 0.018f * (float) wave + 0.16f * (1.0f - (float) n / (float) total);
        bool edge = false;
        for (auto& e : enemies) if (e.alive) { e.x += dir * speed * dt; if (e.x < 0.04f || e.x > 0.96f) edge = true; }
        if (edge) { dir = -dir; for (auto& e : enemies) { e.x = juce::jlimit (0.04f, 0.96f, e.x); e.y += boss ? 0.0f : 0.028f; } }
        for (auto& e : enemies) if (e.alive && e.y > playerY - 0.06f) { lives = 0; over = true; ev.push_back ({ evGameOver, px, playerY }); return ev; }
        // enemy fire: the lowest alive enemy of a random column
        if (n > 0 && rng.uni() < (0.45f + 0.2f * (float) wave) * dt)
        {
            const int col = (int) (rng.next() % 8u); const Enemy* low = nullptr;
            for (auto& e : enemies) if (e.alive && e.col == col && (low == nullptr || e.y > low->y)) low = &e;
            if (low != nullptr) shots.push_back ({ low->x, low->y + 0.03f, 0, 0.55f + 0.04f * (float) wave, true });
        }
        if (boss && bossHp > 0)
        {
            bossX = 0.5f + 0.32f * std::sin (t * (0.7f + 0.08f * (float) level()));
            bossY = 0.15f + 0.03f * std::sin (t * 2.3f);
            bossFire -= dt;
            if (bossFire <= 0)
            {
                bossFire = std::max (0.45f, 1.2f - 0.1f * (float) level());
                for (int k = -1; k <= 1; ++k) shots.push_back ({ bossX + 0.04f * (float) k, bossY + bossH * 0.5f, 0.22f * (float) k, 0.5f, true });
            }
        }
        // shots move and hit
        for (auto& s : shots) { s.x += s.vx * dt; s.y += s.vy * dt; }
        for (auto& s : shots)
        {
            if (s.y < -0.05f || s.y > 1.05f) { s.y = 9; continue; }
            if (! s.enemy)
            {
                for (auto& e : enemies)
                    if (e.alive && std::abs (s.x - e.x) < enemyW * 0.5f && std::abs (s.y - e.y) < enemyH * 0.5f)
                    {
                        e.alive = false; s.y = 9; ++kills;
                        const int pts = 10 * (rows - e.row) + 5 * combo;
                        score += pts;
                        ev.push_back ({ evKill, e.x, e.y, killPitch (e), juce::jlimit (0.35f, 1.0f, 0.6f + 0.08f * (float) combo), pts });
                        ++combo; comboT = 0.9f;
                        break;
                    }
                if (s.y < 9 && boss && bossHp > 0 && std::abs (s.x - bossX) < bossW * 0.5f && std::abs (s.y - bossY) < bossH * 0.5f)
                {
                    s.y = 9; bossHp -= 1.0f; bossFlash = 1.0f; score += 5;
                    if (bossHp <= 0)
                    {
                        score += 500 + 100 * level();
                        ev.push_back ({ evBossKill, bossX, bossY, key.pitch (-5, 0.0f), 1.0f, 500 });
                    }
                    else ev.push_back ({ evBossHit, bossX, bossY, key.pitch (-5 + (int) (bossHp) % 3, 0.0f), 0.55f + 0.4f * (1.0f - bossHp / bossMax), 5 });   // the 808 boss: low notes
                }
            }
            else if (invuln <= 0 && std::abs (s.x - px) < 0.035f && s.y > playerY - 0.025f && s.y < playerY + 0.03f)
            {
                s.y = 9; --lives; invuln = 1.4f; combo = 0;
                ev.push_back ({ evPlayerHit, px, playerY, key.pitch (-3, 0.0f), 0.9f });
                if (lives <= 0) { over = true; ev.push_back ({ evGameOver, px, playerY }); return ev; }
            }
        }
        shots.erase (std::remove_if (shots.begin(), shots.end(), [] (const Shot& s) { return s.y > 5; }), shots.end());
        const bool bossDone = ! boss || bossHp <= 0;
        if (alive() == 0 && bossDone)
        {
            ev.push_back ({ evWaveClear, 0.5f, 0.5f, key.pitch (7), 0.9f, wave });
            if (boss) { victory = true; over = true; return ev; }      // the 808 BOSS is down: level clear -> the chest
            ++wave; spawnWave();
        }
        return ev;
    }
};

// ================================================================ PIXEL DUEL ================================================================
// 1v1 against the CPU on a flat arena (x 0..1, y = height above the ground).  move, jump, punch, special (a sound wave).
// every hit sounds: your hits in your register, the rival's lower.  win = steal the rival's sound.
struct Duel
{
    enum Act { aIdle, aWalk, aJump, aPunch, aSpecial, aHurt, aKo };
    enum EvKind { evHit, evSpecialHit, evWhiff, evKo, evSpecialFire, evTimeUp };
    struct Input { bool left = false, right = false, jump = false, punch = false, special = false; };
    struct Fighter { float x = 0.3f, y = 0, vx = 0, vy = 0; int facing = 1; float hp = 100; int act = aIdle; float actT = 0, cd = 0, charge = 0, hurt = 0; bool landed = false; int hits = 0; };
    struct Wave { float x = 0, y = 0, vx = 0; int owner = 0; bool live = true; };
    static constexpr float halfW = 0.04f, punchRange = 0.115f, punchTime = 0.3f, gravity = 5.2f;
    Key key; Fighter f[2]; std::vector<Wave> waves; Rng ai[2];
    float t = 0, timeLeft = 60; int winner = -1, level = 1, combo[2] {};
    float aiT[2] {}; Input aiHold[2];

    void start (int lvl, uint32_t seed, Key k)
    {
        key = k; level = std::max (1, lvl); t = 0; timeLeft = 60; winner = -1; waves.clear();
        f[0] = {}; f[1] = {}; f[0].x = 0.28f; f[1].x = 0.72f; f[0].facing = 1; f[1].facing = -1;
        f[1].hp = 90.0f + 10.0f * (float) std::min (level, 6);
        ai[0].seed (hash32 (seed + 11u)); ai[1].seed (hash32 (seed * 3u + (uint32_t) level));
        aiT[0] = aiT[1] = 0; combo[0] = combo[1] = 0;
    }
    float maxHp (int who) const { return who == 0 ? 100.0f : 90.0f + 10.0f * (float) std::min (level, 6); }
    // the CPU: walks in, punches in range, jumps over waves, fires its special when charged; reacts faster on higher levels
    Input cpu (int who, float dt)
    {
        auto& me = f[who]; const auto& op = f[1 - who];
        aiT[who] -= dt;
        if (aiT[who] > 0) return aiHold[who];
        Input in; auto& r = ai[who];
        const float skill = juce::jlimit (0.2f, 0.95f, 0.35f + 0.1f * (float) level);
        aiT[who] = (0.22f - 0.12f * skill) + 0.1f * r.uni();
        const float d = op.x - me.x, ad = std::abs (d);
        bool waveComing = false;
        for (auto& w : waves) if (w.live && w.owner != who && std::abs (w.x - me.x) < 0.25f && (w.vx > 0) == (me.x > w.x)) waveComing = true;
        if (waveComing && me.y <= 0 && r.uni() < skill) in.jump = true;
        if (me.charge >= 1.0f && ad > 0.2f && r.uni() < 0.6f) in.special = true;
        else if (ad < punchRange * 0.95f) { if (r.uni() < 0.35f + 0.5f * skill) in.punch = true; else if (r.uni() < 0.3f) (d > 0 ? in.left : in.right) = true; }
        else if (r.uni() < 0.85f) (d > 0 ? in.right : in.left) = true;
        if (r.uni() < 0.04f) in.jump = true;
        aiHold[who] = in;
        return in;
    }
    std::vector<Ev> step (float dt, Input p0, Input p1)
    {
        std::vector<Ev> ev;
        if (winner >= 0) return ev;
        t += dt; timeLeft -= dt;
        const Input in[2] { p0, p1 };
        for (int i = 0; i < 2; ++i)
        {
            auto& me = f[i]; auto& op = f[1 - i];
            me.cd = std::max (0.0f, me.cd - dt); me.hurt = std::max (0.0f, me.hurt - dt);
            const bool busy = me.act == aPunch || me.act == aSpecial || me.hurt > 0;
            float move = 0;
            if (! busy) move = (in[i].right ? 1.0f : 0.0f) - (in[i].left ? 1.0f : 0.0f);
            me.vx = move * 0.42f + (me.hurt > 0 ? me.vx : 0.0f);
            if (! busy && in[i].jump && me.y <= 0) { me.vy = 1.55f; me.y = 0.0001f; }
            if (! busy && in[i].punch && me.cd <= 0) { me.act = aPunch; me.actT = 0; me.landed = false; me.cd = 0.36f; }
            if (! busy && in[i].special && me.cd <= 0 && me.charge >= 1.0f)
            {
                me.act = aSpecial; me.actT = 0; me.cd = 0.6f; me.charge = 0;
                waves.push_back ({ me.x + (float) me.facing * 0.06f, me.y + 0.08f, (float) me.facing * 0.85f, i, true });
                ev.push_back ({ evSpecialFire, me.x, me.y, -1, 0.6f, i });
            }
            me.x = juce::jlimit (0.05f, 0.95f, me.x + me.vx * dt);
            if (me.y > 0 || me.vy > 0) { me.vy -= gravity * dt; me.y += me.vy * dt; if (me.y <= 0) { me.y = 0; me.vy = 0; } }
            if (me.act != aPunch && me.act != aSpecial && me.hurt <= 0) me.facing = op.x >= me.x ? 1 : -1;
            // the punch lands in its active window
            if (me.act == aPunch)
            {
                me.actT += dt;
                if (! me.landed && me.actT > 0.08f && me.actT < 0.18f)
                {
                    const float reach = (op.x - me.x) * (float) me.facing;
                    if (reach > 0 && reach < punchRange && std::abs (op.y - me.y) < 0.14f)
                    {
                        me.landed = true;
                        const float dmg = 6.0f + 0.6f * (float) std::min (level, 6) * (i == 1 ? 1.0f : 0.5f) + (float) std::min (combo[i], 3);
                        hit (i, dmg, false, ev);
                    }
                }
                if (me.actT >= punchTime) { if (! me.landed) { combo[i] = 0; ev.push_back ({ evWhiff, me.x, me.y, -1, 0.3f, i }); } me.act = aIdle; }
            }
            else if (me.act == aSpecial) { me.actT += dt; if (me.actT > 0.35f) me.act = aIdle; }
            else if (me.hurt > 0) me.act = aHurt;
            else me.act = me.y > 0 ? aJump : std::abs (me.vx) > 0.01f ? aWalk : aIdle;
        }
        // fighters do not walk through each other
        if (std::abs (f[0].x - f[1].x) < halfW * 1.6f && std::abs (f[0].y - f[1].y) < 0.12f)
        {
            const float mid = (f[0].x + f[1].x) * 0.5f, s = f[0].x <= f[1].x ? -1.0f : 1.0f;
            f[0].x = juce::jlimit (0.05f, 0.95f, mid + s * halfW * 0.8f); f[1].x = juce::jlimit (0.05f, 0.95f, mid - s * halfW * 0.8f);
        }
        for (auto& w : waves)
        {
            if (! w.live) continue;
            w.x += w.vx * dt;
            auto& op = f[1 - w.owner];
            if (std::abs (w.x - op.x) < 0.045f && op.y < 0.12f) { w.live = false; hit (w.owner, 15.0f + (float) std::min (level, 6), true, ev); }
            if (w.x < -0.05f || w.x > 1.05f) w.live = false;
        }
        waves.erase (std::remove_if (waves.begin(), waves.end(), [] (const Wave& w) { return ! w.live; }), waves.end());
        for (int i = 0; i < 2; ++i)
            if (f[i].hp <= 0) { f[i].hp = 0; f[i].act = aKo; winner = 1 - i; ev.push_back ({ evKo, f[i].x, f[i].y, key.pitch (i == 0 ? -7 : 0, 0.0f), 1.0f, winner }); return ev; }
        if (timeLeft <= 0) { timeLeft = 0; winner = f[0].hp / maxHp (0) > f[1].hp / maxHp (1) ? 0 : 1; ev.push_back ({ evTimeUp, 0.5f, 0, -1, 0.8f, winner }); }
        return ev;
    }
    void hit (int who, float dmg, bool special, std::vector<Ev>& ev)
    {
        auto& me = f[who]; auto& op = f[1 - who];
        op.hp -= dmg; op.hurt = special ? 0.45f : 0.24f; op.vx = (float) me.facing * (special ? 0.5f : 0.3f);
        me.charge = std::min (1.0f, me.charge + (special ? 0.0f : 0.26f)); op.charge = std::min (1.0f, op.charge + 0.12f);
        ++me.hits; ++combo[who]; combo[1 - who] = 0;
        // your hits sing in your register, climbing with the combo; the rival answers an octave below
        const int deg = special ? 7 : (combo[who] - 1) % 6;
        ev.push_back ({ special ? evSpecialHit : evHit, op.x, op.y + 0.1f, key.pitch (deg, who == 0 ? 0.5f : 0.0f), juce::jlimit (0.3f, 1.0f, dmg / 18.0f), who });
    }
};

// ================================================================ PINBALL ================================================================
// table 0..1 x 0..1.6 (y down).  hold = the plunger charges, release = launch; two flippers; bumpers are notes in the key
// and INGREDIENTS - when the last ball drains, the run cooks a new sound from what it hit.  the bounces are a melody (DRAG MIDI).
enum Ingredient { inMetal, inGlass, inBone, inGas, inCrystal, inLava, inSpark, inBreath, inMud, numIngredients };
struct IngredientInfo { const char* name; juce::uint32 colour; int exc, body; float matter, size; };
inline const IngredientInfo& ingredient (int i)
{
    static const IngredientInfo t[] {
        { "METAL",   0xffd8dde6,  0, -1, -1.0f, -1.0f }, { "GLASS", 0xff9be7ff, -1,  3, 0.04f, -1.0f }, { "BONE",  0xfff1e3c6, -1,  0, -1.0f, -1.0f },
        { "GAS",     0xffa78bfa, -1,  1, -1.0f, 0.9f },  { "CRYSTAL", 0xff7df9ff, -1, 3, 0.15f, 0.12f }, { "LAVA",  0xffff3b5c,  4,  4, 0.7f, -1.0f },
        { "SPARK",   0xff5ad1ff,  2, -1, -1.0f, 0.2f },  { "BREATH", 0xffb4f0c8,  3, -1, -1.0f, -1.0f }, { "MUD",   0xff8a5a2b, -1,  2, 0.96f, 0.75f } };
    return t[juce::jlimit (0, (int) numIngredients - 1, i)];
}

struct Pinball
{
    enum EvKind { evBumper, evSling, evRamp, evFlipper, evDrain, evLaunch, evGameOver, evWall };
    struct Point2 { float x, y; };
    struct Seg { float x0, y0, x1, y1; float e; int kind; int ing; };            // kind 0 wall, 1 slingshot
    struct Bumper { float x, y, r; int ing; int degree; float flash = 0; };
    struct Ramp { float x0, y0, x1, y1; int ing; int degree; float cd = 0, flash = 0; };
    struct Flipper { float px, py, len; bool left; float ang, w = 0; };
    static constexpr float W = 1.0f, H = 1.6f, R = 0.024f, flipRest = 0.52f, flipUp = -0.45f, flipR = 0.017f, laneX = 0.92f, plungerY = 1.54f;
    Key key; Rng rng;
    std::vector<Seg> segs; std::vector<Bumper> bumpers; std::vector<Ramp> ramps; std::array<Flipper, 2> fl {};
    float bx = 0, by = 0, vx = 0, vy = 0, t = 0, power = 0, still = 0;
    bool inLane = true, charging = false, over = false;
    int balls = 3, score = 0, hitsTotal = 0;
    std::array<int, numIngredients> cooked {};
    std::vector<kk::live::LNote> notes;
    uint32_t hitHash = 0x811c9dc5u;

    void start (uint32_t seed, Key k)
    {
        key = k; rng.seed (hash32 (seed + 99u)); t = 0; balls = 3; score = 0; over = false; cooked = {}; notes.clear(); hitsTotal = 0; hitHash = 0x811c9dc5u;
        segs = {
            { 0.0f, 0.24f, 0.0f, 1.18f, 0.5f, 0, -1 }, { 0.0f, 0.24f, 0.06f, 0.1f, 0.5f, 0, -1 }, { 0.06f, 0.1f, 0.18f, 0.02f, 0.5f, 0, -1 },
            { 0.18f, 0.02f, 0.8f, 0.02f, 0.5f, 0, -1 }, { 0.8f, 0.02f, 0.93f, 0.07f, 0.5f, 0, -1 }, { 0.93f, 0.07f, 1.0f, 0.2f, 0.5f, 0, -1 },
            { 1.0f, 0.2f, 1.0f, 1.6f, 0.3f, 0, -1 }, { laneX, 0.42f, laneX, 1.6f, 0.3f, 0, -1 }, { laneX, plungerY, 1.0f, plungerY, 0.1f, 0, -1 },
            { 0.0f, 1.18f, 0.3f, 1.4f, 0.4f, 0, -1 }, { laneX, 1.18f, 0.7f, 1.4f, 0.4f, 0, -1 },
            { 0.1f, 1.0f, 0.2f, 1.2f, 0.9f, 1, inMud }, { 0.82f, 1.0f, 0.72f, 1.2f, 0.9f, 1, inBreath } };   // the two slingshots
        bumpers = { { 0.3f, 0.36f, 0.058f, inMetal, 0 }, { 0.56f, 0.3f, 0.058f, inGlass, 2 }, { 0.44f, 0.54f, 0.058f, inBone, 4 },
                    { 0.7f, 0.52f, 0.052f, inCrystal, 5 }, { 0.22f, 0.68f, 0.05f, inLava, 1 } };
        ramps = { { 0.04f, 0.34f, 0.16f, 0.44f, inGas, 7, 0, 0 }, { 0.72f, 0.12f, 0.84f, 0.22f, inSpark, 9, 0, 0 } };
        fl[0] = { 0.3f, 1.4f, 0.17f, true, flipRest, 0 }; fl[1] = { 0.7f, 1.4f, 0.17f, false, flipRest, 0 };
        resetBall();
    }
    void resetBall() { bx = 0.96f; by = plungerY - R - 0.001f; vx = vy = 0; inLane = true; power = 0; charging = false; still = 0; }
    Point2 tip (int i) const { const auto& f = fl[(size_t) i]; return { f.px + (f.left ? 1.0f : -1.0f) * f.len * std::cos (f.ang), f.py + f.len * std::sin (f.ang) }; }
    float beats() const { return t * 2.0f; }                                                              // 120 BPM
    void addNote (int pitch, float vel, float len)
    {
        if (notes.size() >= 400) return;
        const float b = std::round (beats() * 4.0f) / 4.0f;
        if (b >= 64.0f) return;
        if (! notes.empty() && notes.back().start == b && notes.back().pitch == pitch) return;
        notes.push_back ({ b, len, pitch, juce::jlimit (0.15f, 1.0f, vel), 0 });
    }
    void cook (int ing) { ++cooked[(size_t) ing]; ++hitsTotal; hitHash = (hitHash ^ (uint32_t) (ing + 1)) * 16777619u; }

    // collide the ball with a capsule (a segment with a radius) that may move (vs = the surface velocity at the contact)
    bool collide (float x0, float y0, float x1, float y1, float rad, float e, float svx, float svy, float& impact)
    {
        const float lx = x1 - x0, ly = y1 - y0, len2 = lx * lx + ly * ly;
        const float u = len2 > 1.0e-9f ? juce::jlimit (0.0f, 1.0f, ((bx - x0) * lx + (by - y0) * ly) / len2) : 0.0f;
        const float cx = x0 + u * lx, cy = y0 + u * ly, dx = bx - cx, dy = by - cy, d2 = dx * dx + dy * dy, rr = R + rad;
        if (d2 >= rr * rr || d2 < 1.0e-12f) return false;
        const float d = std::sqrt (d2), nx = dx / d, ny = dy / d;
        bx = cx + nx * rr; by = cy + ny * rr;
        const float rvn = (vx - svx) * nx + (vy - svy) * ny;
        impact = -rvn;
        if (rvn < 0) { vx -= (1.0f + e) * rvn * nx; vy -= (1.0f + e) * rvn * ny; }
        return true;
    }
    std::vector<Ev> step (float dt, bool leftFlip, bool rightFlip, bool plunger)
    {
        std::vector<Ev> ev;
        if (over) return ev;
        const int sub = std::max (1, (int) std::ceil (dt / (1.0f / 600.0f)));
        const float h = dt / (float) sub;
        for (auto& b : bumpers) b.flash = std::max (0.0f, b.flash - dt * 3);
        for (auto& r : ramps) { r.flash = std::max (0.0f, r.flash - dt * 2); r.cd = std::max (0.0f, r.cd - dt); }
        // the plunger: hold = charge, release = launch
        if (inLane)
        {
            if (plunger) { charging = true; power = std::min (1.0f, power + dt * 1.1f); }
            else if (charging) { charging = false; vy = -(1.7f + 2.6f * power); ev.push_back ({ evLaunch, bx, by, -1, power }); power = 0; inLane = false; }
        }
        for (int s = 0; s < sub; ++s)
        {
            t += h;
            for (int i = 0; i < 2; ++i)
            {
                auto& f = fl[(size_t) i];
                const bool up = i == 0 ? leftFlip : rightFlip;
                const float target = up ? flipUp : flipRest, prev = f.ang;
                const float spd = up ? 22.0f : 12.0f;
                f.ang = target < f.ang ? std::max (target, f.ang - spd * h) : std::min (target, f.ang + spd * h);
                f.w = (f.ang - prev) / h;
            }
            if (inLane && vy == 0 && vx == 0) continue;      // resting on the plunger
            vy += 2.2f * h;
            const float sp = std::sqrt (vx * vx + vy * vy);
            if (sp > 4.6f) { vx *= 4.6f / sp; vy *= 4.6f / sp; }
            const float px = bx, py = by;
            bx += vx * h; by += vy * h;
            float imp = 0;
            for (auto& g : segs)
                if (collide (g.x0, g.y0, g.x1, g.y1, 0.004f, g.e, 0, 0, imp) && g.kind == 1 && imp > 0.25f)
                {
                    // a slingshot kicks the ball away
                    const float nx = -(g.y1 - g.y0), ny = g.x1 - g.x0, nl = std::sqrt (nx * nx + ny * ny);
                    float kx = nx / nl, ky = ny / nl; if ((bx - g.x0) * kx + (by - g.y0) * ky < 0) { kx = -kx; ky = -ky; }
                    vx += kx * 1.1f; vy += ky * 1.1f;
                    score += 10; cook (g.ing);
                    const int p = key.pitch (g.ing == inMud ? -2 : 3); const float vel = std::min (1.0f, imp / 3.0f + 0.3f);
                    addNote (p, vel, 0.25f); ev.push_back ({ evSling, bx, by, p, vel, g.ing });
                }
            for (int bi = 0; bi < (int) bumpers.size(); ++bi)
            {
                auto& b = bumpers[(size_t) bi];
                const float dx = bx - b.x, dy = by - b.y, d2 = dx * dx + dy * dy, rr = R + b.r;
                if (d2 >= rr * rr || d2 < 1.0e-12f) continue;
                const float d = std::sqrt (d2), nx = dx / d, ny = dy / d;
                bx = b.x + nx * rr; by = b.y + ny * rr;
                const float vn = vx * nx + vy * ny;
                const float hitSp = std::max (0.0f, -vn);
                if (vn < 0) { vx -= 1.9f * vn * nx; vy -= 1.9f * vn * ny; }
                const float out = vx * nx + vy * ny;
                if (out < 1.5f) { vx += nx * (1.5f - out); vy += ny * (1.5f - out); }   // the bumper pops
                b.flash = 1; score += 100; cook (b.ing);
                const float vel = juce::jlimit (0.2f, 1.0f, 0.25f + hitSp / 3.2f);         // hit strength = velocity
                const int p = key.pitch (b.degree);
                addNote (p, vel, 0.5f); ev.push_back ({ evBumper, b.x, b.y, p, vel, bi });
            }
            for (int i = 0; i < 2; ++i)
            {
                const auto& f = fl[(size_t) i]; const auto tp = tip (i);
                // the surface velocity of the flipper where the ball touches it
                const float lx = tp.x - f.px, ly = tp.y - f.py, len2 = lx * lx + ly * ly;
                const float u = juce::jlimit (0.0f, 1.0f, ((bx - f.px) * lx + (by - f.py) * ly) / len2);
                const float cx = f.px + u * lx - f.px, cy = f.py + u * ly - f.py;
                const float svx = f.left ? -f.w * cy : f.w * cy, svy = f.left ? f.w * cx : -f.w * cx;
                if (collide (f.px, f.py, tp.x, tp.y, flipR, 0.3f, svx, svy, imp) && std::abs (f.w) > 1.0f && imp > 0.8f)
                    ev.push_back ({ evFlipper, bx, by, -1, std::min (1.0f, imp / 4.0f), i });
            }
            // ramps: sensors you shoot through upwards
            for (int ri = 0; ri < (int) ramps.size(); ++ri)
            {
                auto& r = ramps[(size_t) ri];
                if (r.cd > 0 || vy > -0.4f) continue;
                if (bx > r.x0 && bx < r.x1 && by > r.y0 && by < r.y1 && ! (py > r.y0 && py < r.y1 && px > r.x0 && px < r.x1))
                {
                    r.cd = 0.8f; r.flash = 1; score += 500; cook (r.ing);
                    const float vel = juce::jlimit (0.3f, 1.0f, 0.3f + std::abs (vy) / 4.0f);
                    const int p = key.pitch (r.degree);
                    addNote (p, vel, 1.0f); ev.push_back ({ evRamp, bx, by, p, vel, ri });
                }
            }
            // safety: never leave the table
            if (bx < R) { bx = R; vx = std::abs (vx) * 0.5f; } if (bx > W - R) { bx = W - R; vx = -std::abs (vx) * 0.5f; }
            if (by < R) { by = R; vy = std::abs (vy) * 0.5f; }
            if (bx > laneX && by > 1.3f && by >= plungerY - R - 0.003f && std::abs (vy) < 0.15f) { by = plungerY - R - 0.001f; vx = vy = 0; inLane = true; }   // back on the plunger
            if (by > H - R * 0.5f || (by > 1.5f && bx < laneX - R))
            {
                ev.push_back ({ evDrain, bx, by, key.pitch (-5, 0.0f), 0.7f, balls });
                if (--balls <= 0) { by = juce::jmin (by, H - R); over = true; ev.push_back ({ evGameOver, bx, by }); return ev; }
                resetBall(); break;
            }
        }
        // a stuck ball gets a nudge (TILT)
        if (! inLane && vx * vx + vy * vy < 0.0004f) { still += dt; if (still > 2.5f) { vx = rng.bi() * 1.2f; vy = -1.2f; still = 0; } } else still = 0;
        return ev;
    }
    // the dish: what the run hit decides the recipe
    struct Dish { int exciter = 1, body = 2; float matter = 0.4f, size = 0.5f; uint32_t seed = 1; juce::String name; };
    Dish dish() const
    {
        Dish d; std::array<float, 5> ex {}, bo {}; float mw = 1, ms = 0.4f, sw = 1, ss = 0.5f;
        for (int i = 0; i < numIngredients; ++i)
        {
            const auto& g = ingredient (i); const float n = (float) cooked[(size_t) i];
            if (n <= 0) continue;
            if (g.exc >= 0) ex[(size_t) g.exc] += n; if (g.body >= 0) bo[(size_t) g.body] += n;
            if (g.matter >= 0) { mw += n * 0.3f; ms += n * 0.3f * g.matter; } if (g.size >= 0) { sw += n * 0.3f; ss += n * 0.3f * g.size; }
        }
        auto arg = [] (const std::array<float, 5>& a, int def) { int b = def; float bv = 0; for (int i = 0; i < 5; ++i) if (a[(size_t) i] > bv) { bv = a[(size_t) i]; b = i; } return b; };
        d.exciter = arg (ex, 1); d.body = arg (bo, 2); d.matter = juce::jlimit (0.0f, 1.0f, ms / mw); d.size = juce::jlimit (0.0f, 1.0f, ss / sw);
        d.seed = hash32 (hitHash + (uint32_t) score) | 1u;
        int top = -1, topN = 0, sec = -1, secN = 0;
        for (int i = 0; i < numIngredients; ++i) { const int n = cooked[(size_t) i]; if (n > topN) { sec = top; secN = topN; top = i; topN = n; } else if (n > secN) { sec = i; secN = n; } }
        auto word = [] (int i) { return juce::String (ingredient (i).name).toLowerCase().replaceSection (0, 1, juce::String (ingredient (i).name).substring (0, 1)); };
        d.name = top < 0 ? juce::String ("Empty Plate") : sec < 0 ? "Cooked " + word (top) : word (top) + " & " + word (sec) + " Stew";
        return d;
    }
    kk::live::Result melody() const
    {
        kk::live::Result r; r.notes = notes;
        float end = 4; for (auto& n : notes) end = std::max (end, n.start + n.len);
        r.beats = std::min (64.0f, std::ceil (end / 4.0f) * 4.0f);
        kk::live::tidy (r, 4);
        return r;
    }
};

// ================================================================ CHEST + COLLECTION ================================================================
enum Rarity { rCommon, rRare, rEpic, rLegendary, numRarities };
constexpr int numSlots = 250;
inline const char* rarityName (int r) { static const char* n[] { "COMMON", "RARE", "EPIC", "LEGENDARY" }; return n[juce::jlimit (0, 3, r)]; }
inline juce::uint32 rarityColour (int r) { static const juce::uint32 c[] { 0xffb8c4d6, 0xff3fa9ff, 0xffc04dff, 0xffffc63a }; return c[juce::jlimit (0, 3, r)]; }
inline int slotStart (int r) { static const int s[] { 0, 130, 200, 235, 250 }; return s[juce::jlimit (0, 4, r)]; }
inline int rarityOfSlot (int slot) { for (int r = numRarities - 1; r >= 0; --r) if (slot >= slotStart (r)) return r; return 0; }

// rarer = higher score: s is the run's score normalised 0..1.  legendary is rare at any score but never impossible
inline float pLegendary (float s) { return 0.004f + 0.05f * s * s * s; }
inline int rollRarity (uint32_t h, float s)
{
    s = juce::jlimit (0.0f, 1.0f, s);
    const float u = (float) (hash32 (h) >> 8) * (1.0f / 16777216.0f);
    const float pl = pLegendary (s), pe = 0.04f + 0.22f * s * s, pr = 0.22f + 0.25f * s;
    return u < pl ? rLegendary : u < pl + pe ? rEpic : u < pl + pe + pr ? rRare : rCommon;
}

struct Recipe
{
    int slot = -1, rarity = rCommon, exciter = 0, body = 0;
    float matter = 0.5f, size = 0.5f, stretch = 0, bright = 0, heat = 0, cool = 0, split = 0;
    uint32_t seed = 1; juce::String name; bool cooked = false, stolen = false;
};

inline juce::String slotName (int slot)
{
    static const char* adj[] { "Dusty", "Neon", "Velvet", "Rusty", "Frozen", "Hollow", "Liquid", "Static", "Molten", "Silent", "Feral", "Lunar", "Glitch", "Amber", "Ghost", "Thunder" };
    static const char* noun[] { "Moth", "Pebble", "Comet", "Lantern", "Oracle", "Beetle", "Harp", "Tide", "Engine", "Petal", "Golem", "Signal", "Ember", "Vapor", "Choir", "Crown" };
    const int a = slot % 16, b = (slot / 16 + 5 * a) % 16;
    return juce::String (adj[a]) + " " + noun[b];
}

// the recipe of a collection slot.  rarity widens the alchemy ranges: legendary = extreme matter / size, special seeds,
// a SCULPT twist (torn + hot or frozen) and a golden name
inline Recipe recipeForSlot (int slot)
{
    slot = juce::jlimit (0, numSlots - 1, slot);
    Recipe r; r.slot = slot; r.rarity = rarityOfSlot (slot);
    Rng g; g.seed (hash32 ((uint32_t) slot * 7919u + 13u));
    r.exciter = (int) (g.next() % 5u); r.body = (int) (g.next() % 5u);
    r.seed = hash32 ((uint32_t) slot * 2654435761u + 17u) | 1u;
    r.name = slotName (slot);
    switch (r.rarity)
    {
        case rCommon:    r.matter = 0.25f + 0.5f * g.uni(); r.size = 0.3f + 0.4f * g.uni(); r.bright = 0.4f * g.bi() * 0.5f; break;
        case rRare:      r.matter = 0.1f + 0.8f * g.uni(); r.size = 0.15f + 0.7f * g.uni(); r.stretch = 0.4f * g.bi(); r.bright = 0.4f * g.bi();
                         (g.uni() < 0.5f ? r.heat : r.cool) = 0.3f * g.uni(); break;
        case rEpic:      r.matter = g.uni(); r.size = g.uni(); r.stretch = 0.8f * g.bi(); r.bright = 0.6f * g.bi(); r.heat = 0.6f * g.uni(); r.cool = 0.6f * g.uni(); r.split = 0.5f * g.uni(); break;
        default:
            r.matter = g.uni() < 0.5f ? 0.06f * g.uni() : 0.94f + 0.06f * g.uni();
            r.size = g.uni() < 0.5f ? 0.1f * g.uni() : 0.9f + 0.1f * g.uni();
            r.seed = 0xA1C0DE00u + (uint32_t) slot;                                      // special seeds
            r.stretch = g.uni() < 0.5f ? -0.8f - 0.2f * g.uni() : 0.8f + 0.2f * g.uni();
            r.split = 0.7f + 0.3f * g.uni();
            if (g.uni() < 0.5f) r.heat = 0.8f + 0.2f * g.uni(); else r.cool = 0.8f + 0.2f * g.uni();
            r.name = "Golden " + r.name;
            break;
    }
    return r;
}

// how good a score is (0..1) for each game
inline float scoreNorm (int game, int score) { static const float full[] { 3000.0f, 1500.0f, 25000.0f }; return juce::jlimit (0.0f, 1.0f, (float) score / full[juce::jlimit (0, 2, game)]); }
inline int scoreBand (int game, int score) { static const int band[] { 150, 100, 1500 }; return std::max (0, score) / band[juce::jlimit (0, 2, game)]; }
// a chest worth opening: a cleared level, or a good enough score
inline bool earnsChest (int game, int score, bool levelDone) { static const int need[] { 300, 200, 2500 }; return levelDone || score >= need[juce::jlimit (0, 2, game)]; }

struct Chest { int game = 0, level = 1, score = 0; std::vector<Recipe> items; int best() const { int b = 0; for (auto& r : items) b = std::max (b, r.rarity); return b; } };
inline Chest openChest (int game, int level, int score, bool levelDone)
{
    Chest c; c.game = game; c.level = level; c.score = score;
    const float s = juce::jlimit (0.0f, 1.0f, scoreNorm (game, score) + (levelDone ? 0.1f : 0.0f) + 0.03f * (float) (level - 1));
    const int n = juce::jlimit (1, 5, 1 + (s > 0.15f) + (s > 0.35f) + (s > 0.6f) + (s > 0.85f));
    const uint32_t base = hash32 ((uint32_t) game * 1000003u + (uint32_t) level * 7919u + (uint32_t) scoreBand (game, score) * 104729u + (levelDone ? 77u : 0u));
    for (int i = 0; i < n; ++i)
    {
        const uint32_t h = hash32 (base + (uint32_t) i * 0x9E3779B9u);
        const int rar = rollRarity (h, s);
        const int span = slotStart (rar + 1) - slotStart (rar);
        int slot = slotStart (rar) + (int) (hash32 (h ^ 0x5bd1e995u) % (uint32_t) span);
        for (auto& o : c.items) if (o.slot == slot) slot = slotStart (rar) + (slot - slotStart (rar) + 1) % span;   // no twins in one chest
        c.items.push_back (recipeForSlot (slot));
    }
    std::sort (c.items.begin(), c.items.end(), [] (const Recipe& a, const Recipe& b) { return a.rarity > b.rarity; });
    return c;
}
// PIXEL DUEL: the rival carries a sound - beat it and it is yours (rare at level 1 ... legendary from level 5)
inline Recipe rivalRecipe (int level)
{
    const int rar = level >= 5 ? rLegendary : level >= 3 ? rEpic : rRare;
    const int span = slotStart (rar + 1) - slotStart (rar);
    auto r = recipeForSlot (slotStart (rar) + (int) (hash32 ((uint32_t) level * 31337u + 5u) % (uint32_t) span));
    r.stolen = true; return r;
}
inline const char* rivalName (int level) { static const char* n[] { "VOLT MOTH", "BRASS GOLEM", "GLASS VIPER", "MUD KING", "NEON ORACLE", "THE SUB TYRANT", "CHROME WITCH", "PIXEL REAPER" }; return n[(std::max (1, level) - 1) % 8]; }
inline juce::uint32 rivalColour (int level) { static const juce::uint32 c[] { 0xff5ad1ff, 0xffffa53d, 0xff7dffb0, 0xffb07a4a, 0xffff4fd8, 0xffff3b3b, 0xffd6e2ff, 0xffa06bff }; return c[(std::max (1, level) - 1) % 8]; }
// PINBALL: the cooked dish as a recipe (rarity by score); not a collection slot - it is your own dish
inline Recipe dishRecipe (const Pinball::Dish& d, int score)
{
    Recipe r; r.cooked = true; r.rarity = rollRarity (d.seed, scoreNorm (gPinball, score));
    r.exciter = d.exciter; r.body = d.body; r.matter = d.matter; r.size = d.size; r.seed = d.seed; r.name = d.name;
    if (r.rarity == rLegendary) { r.split = 0.75f; r.heat = 0.8f; r.name = "Golden " + r.name; }
    return r;
}

// any recipe as a genome (Proc = KeysKillaProcessor: alchemy + sculpt)
template <class Proc> auto genomeFor (Proc& p, const Recipe& r)
{
    auto g = p.sculpt (p.alchemy (r.exciter, r.body, r.matter, r.size, r.seed), r.stretch, r.bright, r.heat, r.cool, r.split);
    if (g.valid()) g.name = r.name;
    return g;
}

// the COLLECTION BOOK: how many times each slot was unlocked (kept in the settings as "gameCollection")
struct Collection
{
    std::array<int, numSlots> count {};
    int unlocked() const { int n = 0; for (auto c : count) n += c > 0; return n; }
    int unlocked (int rarity) const { int n = 0; for (int i = slotStart (rarity); i < slotStart (rarity + 1); ++i) n += count[(size_t) i] > 0; return n; }
    int add (int slot) { if (slot < 0 || slot >= numSlots) return 0; return ++count[(size_t) slot]; }
    juce::String toString() const { juce::String s; for (int i = 0; i < numSlots; ++i) if (count[(size_t) i] > 0) s << i << ":" << count[(size_t) i] << ";"; return s; }
    void fromString (const juce::String& s)
    {
        count = {};
        for (auto& tok : juce::StringArray::fromTokens (s, ";", ""))
            if (tok.containsChar (':')) { const int i = tok.upToFirstOccurrenceOf (":", false, false).getIntValue(); if (i >= 0 && i < numSlots) count[(size_t) i] = std::max (0, tok.fromFirstOccurrenceOf (":", false, false).getIntValue()); }
    }
};
} // namespace kk::game

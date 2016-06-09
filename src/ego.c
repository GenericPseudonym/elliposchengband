#include "angband.h"

#include <assert.h>

extern void ego_create_ring(object_type *o_ptr, int level, int power, int mode);
extern void ego_create_amulet(object_type *o_ptr, int level, int power, int mode);
extern bool obj_create_device(object_type *o_ptr, int level, int power, int mode);
extern void obj_create_weapon(object_type *o_ptr, int level, int power, int mode);
extern void obj_create_armor(object_type *o_ptr, int level, int power, int mode);
extern void obj_create_lite(object_type *o_ptr, int level, int power, int mode);
extern int  ego_choose_type(int type, int level);
extern void ego_weapon_adjust_weight(object_type *o_ptr);

extern void ego_brand_weapon(object_type *o_ptr, int which);
extern void ego_finalize(object_type *o_ptr, int level, int power, int mode);

/*************************************************************************
 * Choose Ego Type
 *************************************************************************/
int apply_magic_ego = 0; /* Hack to force a specific ego type (e.g. quest rewards) */

static int _ego_rarity(ego_type *e_ptr, int level)
{
    int rarity = e_ptr->rarity;
    if (rarity)
    {
        if (e_ptr->max_level && level > e_ptr->max_level)
            rarity += 3*e_ptr->rarity*(level - e_ptr->max_level);
        else if (e_ptr->level && level < e_ptr->level)
            rarity += e_ptr->rarity*(e_ptr->level - level);
    }
    return rarity;
}
static int _ego_weight(ego_type *e_ptr, int level)
{
    int rarity = _ego_rarity(e_ptr, level);
    int weight = 0;
    if (rarity)
        weight = MAX(10000 / rarity, 1);
    return weight;
}
typedef bool (*_ego_p)(int e_idx);

static int _choose_type(int type, int level, _ego_p p)
{
    int i, value;
    ego_type *e_ptr;

    int total = 0;

    for (i = 1; i < max_e_idx; i++)
    {
        if (p && !p(i)) continue;

        e_ptr = &e_info[i];

        if (e_ptr->type & type)
            total += _ego_weight(e_ptr, level);
    }

    value = randint1(total);

    for (i = 1; i < max_e_idx; i++)
    {
        if (p && !p(i)) continue;

        e_ptr = &e_info[i];

        if (e_ptr->type & type)
        {
            value -= _ego_weight(e_ptr, level);
            if (value <= 0)
                return i;
        }
    }

    return 0;
}

static bool _ego_p_ring(int e_idx)
{
    switch (obj_drop_theme)
    {
    case R_DROP_WARRIOR:
    case R_DROP_WARRIOR_SHOOT:
    case R_DROP_SAMURAI:
        if (e_idx == EGO_RING_COMBAT) return TRUE;
        return FALSE;
    case R_DROP_ARCHER:
        if (e_idx == EGO_RING_ARCHERY) return TRUE;
        return FALSE;
    case R_DROP_MAGE:
        if ( e_idx == EGO_RING_PROTECTION
          || e_idx == EGO_JEWELRY_ELEMENTAL
          || e_idx == EGO_JEWELRY_DEFENDER
          || e_idx == EGO_RING_WIZARDRY
          || e_idx == EGO_RING_SPEED )
        {
            return TRUE;
        }
        return FALSE;
    case R_DROP_PRIEST:
    case R_DROP_PRIEST_EVIL:
        if ( e_idx == EGO_RING_PROTECTION
          || e_idx == EGO_JEWELRY_ELEMENTAL
          || e_idx == EGO_JEWELRY_DEFENDER
          || e_idx == EGO_RING_SPEED )
        {
            return TRUE;
        }
        return FALSE;
    case R_DROP_PALADIN:
    case R_DROP_PALADIN_EVIL:
        if ( e_idx == EGO_RING_PROTECTION
          || e_idx == EGO_JEWELRY_DEFENDER
          || e_idx == EGO_RING_SPEED )
        {
            return TRUE;
        }
        return FALSE;
    case R_DROP_ROGUE:
    case R_DROP_NINJA:
        if ( e_idx == EGO_RING_COMBAT
          || e_idx == EGO_RING_ARCHERY
          || e_idx == EGO_RING_SPEED )
        {
            return TRUE;
        }
        return FALSE;
    }
    return TRUE;
}

static bool _ego_p_amulet(int e_idx)
{
    switch (obj_drop_theme)
    {
    case R_DROP_WARRIOR:
    case R_DROP_WARRIOR_SHOOT:
        if ( e_idx == EGO_AMULET_BARBARIAN
          || e_idx == EGO_AMULET_HERO )
        {
            return TRUE;
        }
        return FALSE;
    case R_DROP_DWARF:
        if ( e_idx == EGO_JEWELRY_ELEMENTAL
          || e_idx == EGO_AMULET_DWARVEN )
        {
            return TRUE;
        }
        return FALSE;
    case R_DROP_MAGE:
        if ( e_idx == EGO_JEWELRY_ELEMENTAL
          || e_idx == EGO_JEWELRY_DEFENDER
          || e_idx == EGO_AMULET_MAGI )
        {
            return TRUE;
        }
        return FALSE;
    case R_DROP_PRIEST:
    case R_DROP_PALADIN:
        if ( e_idx == EGO_JEWELRY_ELEMENTAL
          || e_idx == EGO_JEWELRY_DEFENDER
          || e_idx == EGO_AMULET_SACRED
          || e_idx == EGO_AMULET_DEVOTION )
        {
            return TRUE;
        }
        return FALSE;
    case R_DROP_PALADIN_EVIL:
    case R_DROP_PRIEST_EVIL:
        if ( e_idx == EGO_JEWELRY_ELEMENTAL
          || e_idx == EGO_AMULET_HELL )
        {
            return TRUE;
        }
        return FALSE;
    case R_DROP_ROGUE:
    case R_DROP_NINJA:
    case R_DROP_HOBBIT:
        if ( e_idx == EGO_JEWELRY_DEFENDER
          || e_idx == EGO_JEWELRY_ELEMENTAL
          || e_idx == EGO_AMULET_TRICKERY )
        {
            return TRUE;
        }
        return FALSE;
    }
    return TRUE;
}

static bool _ego_p_body_armor(int e_idx)
{
    switch (obj_drop_theme)
    {
    case R_DROP_DWARF:
        if (e_idx == EGO_BODY_DWARVEN) return TRUE;
        return FALSE;
    }
    return TRUE;
}

static bool _ego_p_shield(int e_idx)
{
    switch (obj_drop_theme)
    {
    case R_DROP_DWARF:
        if (e_idx == EGO_SHIELD_DWARVEN) return TRUE;
        return FALSE;
    }
    return TRUE;
}

static bool _ego_p_helmet(int e_idx)
{
    switch (obj_drop_theme)
    {
    case R_DROP_DWARF:
        if (e_idx == EGO_HELMET_DWARVEN) return TRUE;
        return FALSE;
    }
    return TRUE;
}

static bool _ego_p_gloves(int e_idx)
{
    switch (obj_drop_theme)
    {
    case R_DROP_WARRIOR:
    case R_DROP_WARRIOR_SHOOT:
    case R_DROP_SAMURAI:
        if ( e_idx == EGO_GLOVES_SLAYING
          || e_idx == EGO_GLOVES_BERSERKER
          || e_idx == EGO_GLOVES_GIANT
          || e_idx == EGO_GLOVES_GENJI
          || e_idx == EGO_ARMOR_FREE_ACTION )
        {
            return TRUE;
        }
        return FALSE;
    case R_DROP_ARCHER:
        if (e_idx == EGO_GLOVES_SNIPER) return TRUE;
        return FALSE;
    case R_DROP_MAGE:
        if (e_idx == EGO_GLOVES_WIZARD) return TRUE;
        return FALSE;
    case R_DROP_ROGUE:
        if (e_idx == EGO_GLOVES_THIEF) return TRUE;
        return FALSE;
    }
    return TRUE;
}

static bool _ego_p_boots(int e_idx)
{
    switch (obj_drop_theme)
    {
    case R_DROP_DWARF:
        if (e_idx == EGO_BOOTS_DWARVEN) return TRUE;
        return FALSE;
    }
    return TRUE;
}

int ego_choose_type(int type, int level)
{
    _ego_p p = NULL;
    int    e_idx = 0;

    if (apply_magic_ego)
        return apply_magic_ego;

    if (obj_drop_theme)
    {
        switch (type)
        {
        case EGO_TYPE_RING: p = _ego_p_ring; break;
        case EGO_TYPE_AMULET: p = _ego_p_amulet; break;
        case EGO_TYPE_BODY_ARMOR: p = _ego_p_body_armor; break;
        case EGO_TYPE_SHIELD: p = _ego_p_shield; break;
        case EGO_TYPE_HELMET: p = _ego_p_helmet; break;
        case EGO_TYPE_GLOVES: p = _ego_p_gloves; break;
        case EGO_TYPE_BOOTS: p = _ego_p_boots; break;
        }

        /* Hack: Ego creation will often spin a loop, limiting
           certain ego types to certain object types. For example,
           Rusty Chain Mail (Dwarven) or a Pair of Soft Leather Boots (Dwarven)
           is prevented.

           In general, the kind_theme_* hooks should prevent these
           errors, but any mistake would result in an infinite loop.
           Turning off the drop theme here is safe. You just won't get
           a properly themed ego item on the next pass. */
        obj_drop_theme = 0;
    }

    e_idx = _choose_type(type, level, p);
    if (!e_idx && p)
        e_idx = _choose_type(type, level, NULL);
    return e_idx;
}

/*************************************************************************
 * Power Guided Creation Routines
 *************************************************************************/
static void _create_amulet_aux(object_type *o_ptr, int level, int power, int mode);
static void _create_ring_aux(object_type *o_ptr, int level, int power, int mode);

typedef struct { int lvl, min, max; } _power_limit_t;
static _power_limit_t _art_power_limits[] = {
    { 10,     0,   5000 },
    { 20,     0,   7000 },
    { 30,     0,  10000 },
    { 40,     0,  20000 },
    { 50,     0,  30000 },
    { 60, 10000, 100000 },
    { 70, 20000,      0 },
    { 80, 30000,      0 },
    { 90, 40000,      0 },
    {999, 40000,      0 }
};

static void _art_create_random(object_type *o_ptr, int level, int power)
{
    int  i;
    u32b mode = CREATE_ART_NORMAL;
    int  min = 0, max = 0;

    for (i = 0; ; i++)
    {
        if (level < _art_power_limits[i].lvl)
        {
            min = _art_power_limits[i].min;
            max = _art_power_limits[i].max;
            break;
        }
    }
    if (one_in_(GREAT_OBJ))
        max *= 2;

    if (power < 0)
        mode = CREATE_ART_CURSED;

    for (i = 0; i < 1000 ; i++)
    {
        object_type forge = *o_ptr;
        int         score;

        create_artifact(&forge, mode);
        score = object_value_real(&forge);

        if (min > 0 && score < min) continue;
        if (max > 0 && score > max) continue;

        *o_ptr = forge;
        break;
    }
}

static _power_limit_t _jewelry_power_limits[] = {
    { 10,     0,   5000 },
    { 20,     0,   7000 },
    { 30,     0,  10000 },
    { 40,  2500,  20000 },
    { 50,  5000,  30000 },
    { 60,  7500,  60000 },
    { 70, 10000,      0 },
    { 80, 12500,      0 },
    { 90, 15000,      0 },
    {999, 15000,      0 }
};

void ego_create_ring(object_type *o_ptr, int level, int power, int mode)
{
    int  i;
    int  min = 0, max = 0;

    for (i = 0; ; i++)
    {
        if (level < _jewelry_power_limits[i].lvl)
        {
            min = _jewelry_power_limits[i].min;
            max = _jewelry_power_limits[i].max;
            break;
        }
    }
    if (one_in_(GREAT_OBJ))
        max *= 2;

    for (i = 0; i < 1000 ; i++)
    {
        object_type forge = *o_ptr;
        int         score;

        _create_ring_aux(&forge, level, power, mode);
        score = object_value_real(&forge);

        if (min > 0 && score < min) continue;
        if (max > 0 && score > max) continue;

        *o_ptr = forge;
        break;
    }
}

void ego_create_amulet(object_type *o_ptr, int level, int power, int mode)
{
    int  i;
    int  min = 0, max = 0;

    for (i = 0; ; i++)
    {
        if (level < _jewelry_power_limits[i].lvl)
        {
            min = _jewelry_power_limits[i].min;
            max = _jewelry_power_limits[i].max;
            break;
        }
    }
    if (one_in_(GREAT_OBJ))
        max *= 2;

    for (i = 0; i < 1000 ; i++)
    {
        object_type forge = *o_ptr;
        int         score;

        _create_amulet_aux(&forge, level, power, mode);
        score = object_value_real(&forge);

        if (min > 0 && score < min) continue;
        if (max > 0 && score > max) continue;

        *o_ptr = forge;
        break;
    }
}

/*************************************************************************
 * Activations
 *************************************************************************/
#define ACTIVATION_CHANCE (p_ptr->prace == RACE_MON_RING ? 2 : 5)

static int *_effect_list = NULL;
static bool _effect_list_p(int effect)
{
    int i;
    assert(_effect_list);
    for (i = 0; ; i++)
    {
        int n = _effect_list[i];
        if (n == -1) return FALSE;
        if (n == effect) return TRUE;
     }
    /* return FALSE;  unreachable */
}

static void _effect_add_list(object_type *o_ptr, int *list)
{
    _effect_list = list;
    effect_add_random_p(o_ptr, _effect_list_p);
}

/*************************************************************************
 * Jewelry
 *************************************************************************/

static bool _create_level_check(int power, int lvl)
{
    if (lvl <= 0)
        return FALSE;

    /* L/P odds of success ... */
    if (randint0(power * 100 / lvl) < 100)
        return TRUE;

    return FALSE;
}

static int _jewelry_pval(int max, int level)
{
    return randint1(1 + m_bonus(max - 1, level));
}

static int _jewelry_powers(int num, int level, int power)
{
    return abs(power) + m_bonus(num, level);
}

static void _finalize_jewelry(object_type *o_ptr)
{
    if (have_flag(o_ptr->art_flags, TR_RES_ACID))
        add_flag(o_ptr->art_flags, TR_IGNORE_ACID);
    if (have_flag(o_ptr->art_flags, TR_RES_ELEC))
        add_flag(o_ptr->art_flags, TR_IGNORE_ELEC);
    if (have_flag(o_ptr->art_flags, TR_RES_FIRE))
        add_flag(o_ptr->art_flags, TR_IGNORE_FIRE);
    if (have_flag(o_ptr->art_flags, TR_RES_COLD))
        add_flag(o_ptr->art_flags, TR_IGNORE_COLD);
}

static void _ego_create_jewelry_defender(object_type *o_ptr, int level, int power)
{
    add_flag(o_ptr->art_flags, TR_FREE_ACT);
    add_flag(o_ptr->art_flags, TR_SEE_INVIS);
    if (abs(power) >= 2 && level > 50)
    {
        if (one_in_(2))
            add_flag(o_ptr->art_flags, TR_LEVITATION);
        while (one_in_(2))
            one_sustain(o_ptr);
        o_ptr->to_a = 5 + randint1(7) + m_bonus(7, level);
        switch (randint1(4))
        {
        case 1: /* Classic Defender */
            add_flag(o_ptr->art_flags, TR_RES_ACID);
            add_flag(o_ptr->art_flags, TR_RES_ELEC);
            add_flag(o_ptr->art_flags, TR_RES_FIRE);
            add_flag(o_ptr->art_flags, TR_RES_COLD);
            if (one_in_(3))
                add_flag(o_ptr->art_flags, TR_RES_POIS);
            else
                one_high_resistance(o_ptr);
            break;
        case 2: /* High Defender */
            one_high_resistance(o_ptr);
            do
            {
                one_high_resistance(o_ptr);
            }
            while (one_in_(2));
            break;
        case 3: /* Lordly Protection */
            o_ptr->to_a += 5;
            add_flag(o_ptr->art_flags, TR_RES_POIS);
            add_flag(o_ptr->art_flags, TR_RES_DISEN);
            add_flag(o_ptr->art_flags, TR_HOLD_LIFE);
            do
            {
                one_lordly_high_resistance(o_ptr);
            }
            while (one_in_(4));
            break;
        case 4: /* Revenge! */
            add_flag(o_ptr->art_flags, TR_SH_COLD);
            add_flag(o_ptr->art_flags, TR_SH_ELEC);
            add_flag(o_ptr->art_flags, TR_SH_FIRE);
            if (one_in_(2))
                add_flag(o_ptr->art_flags, TR_SH_SHARDS);
            if (one_in_(7))
                add_flag(o_ptr->art_flags, TR_SH_REVENGE);
            break;
        }
    }
    else
    {
        if (one_in_(5))
            add_flag(o_ptr->art_flags, TR_LEVITATION);
        if (one_in_(5))
            one_sustain(o_ptr);
        o_ptr->to_a = randint1(5) + m_bonus(5, level);

        if (one_in_(3))
        {
            one_high_resistance(o_ptr);
            one_high_resistance(o_ptr);
        }
        else
        {
            one_ele_resistance(o_ptr);
            one_ele_resistance(o_ptr);
            one_ele_resistance(o_ptr);
            one_ele_resistance(o_ptr);
            one_ele_resistance(o_ptr);
            one_ele_resistance(o_ptr);
            one_ele_resistance(o_ptr);
        }
    }
    if (one_in_(ACTIVATION_CHANCE))
        effect_add_random(o_ptr, BIAS_PROTECTION);
}
static void _ego_create_jewelry_elemental(object_type *o_ptr, int level, int power)
{
    if (o_ptr->tval == TV_AMULET && abs(power) >= 2 && randint1(level) > 30)
    {
        add_flag(o_ptr->art_flags, TR_RES_COLD);
        add_flag(o_ptr->art_flags, TR_RES_FIRE);
        add_flag(o_ptr->art_flags, TR_RES_ELEC);
        if (one_in_(3))
            add_flag(o_ptr->art_flags, TR_RES_ACID);
        if (one_in_(5))
            add_flag(o_ptr->art_flags, TR_RES_POIS);
        else if (one_in_(5))
            add_flag(o_ptr->art_flags, TR_RES_SHARDS);
    }
    else if (o_ptr->tval == TV_RING && abs(power) >= 2)
    {
        switch (randint1(6))
        {
        case 1:
            add_flag(o_ptr->art_flags, TR_RES_COLD);
            add_flag(o_ptr->art_flags, TR_RES_FIRE);
            add_flag(o_ptr->art_flags, TR_RES_ELEC);
            if (one_in_(3))
                add_flag(o_ptr->art_flags, TR_RES_ACID);
            if (one_in_(5))
                add_flag(o_ptr->art_flags, TR_RES_POIS);
            break;
        case 2:
            o_ptr->to_a = 5 + randint1(5) + m_bonus(10, level);
            add_flag(o_ptr->art_flags, TR_RES_FIRE);
            if (one_in_(3))
                add_flag(o_ptr->art_flags, TR_SH_FIRE);
            if (one_in_(7))
                add_flag(o_ptr->art_flags, TR_BRAND_FIRE);
            else if (randint1(level) >= 70)
                add_flag(o_ptr->art_flags, TR_IM_FIRE);
            if (one_in_(ACTIVATION_CHANCE))
                effect_add_random(o_ptr, BIAS_FIRE);
            break;
        case 3:
            o_ptr->to_a = 5 + randint1(5) + m_bonus(10, level);
            add_flag(o_ptr->art_flags, TR_RES_COLD);
            if (one_in_(3))
                add_flag(o_ptr->art_flags, TR_SH_COLD);
            if (one_in_(7))
                add_flag(o_ptr->art_flags, TR_BRAND_COLD);
            else if (randint1(level) >= 70)
                add_flag(o_ptr->art_flags, TR_IM_COLD);
            if (one_in_(ACTIVATION_CHANCE))
                effect_add_random(o_ptr, BIAS_COLD);
            break;
        case 4:
            o_ptr->to_a = 5 + randint1(5) + m_bonus(10, level);
            add_flag(o_ptr->art_flags, TR_RES_ELEC);
            if (one_in_(3))
                add_flag(o_ptr->art_flags, TR_SH_ELEC);
            if (one_in_(7))
                add_flag(o_ptr->art_flags, TR_BRAND_ELEC);
            else if (randint1(level) >= 75)
                add_flag(o_ptr->art_flags, TR_IM_ELEC);
            if (one_in_(ACTIVATION_CHANCE))
                effect_add_random(o_ptr, BIAS_ELEC);
            break;
        case 5:
            o_ptr->to_a = 5 + randint1(5) + m_bonus(10, level);
            add_flag(o_ptr->art_flags, TR_RES_ACID);
            if (one_in_(7))
                add_flag(o_ptr->art_flags, TR_BRAND_ACID);
            else if (randint1(level) >= 65)
                add_flag(o_ptr->art_flags, TR_IM_ACID);
            if (one_in_(ACTIVATION_CHANCE))
                effect_add_random(o_ptr, BIAS_ACID);
            break;
        case 6:
            o_ptr->to_a = 5 + randint1(5) + m_bonus(10, level);
            add_flag(o_ptr->art_flags, TR_RES_SHARDS);
            if (one_in_(3))
                add_flag(o_ptr->art_flags, TR_SH_SHARDS);
            break;
        }
    }
    else
    {
        one_ele_resistance(o_ptr);
        if (one_in_(3))
            one_ele_resistance(o_ptr);
    }
    if (one_in_(ACTIVATION_CHANCE))
        effect_add_random(o_ptr, BIAS_ELEMENTAL);
}
static void _create_ring_aux(object_type *o_ptr, int level, int power, int mode)
{
    int powers = 0;

    o_ptr->name2 = ego_choose_type(EGO_TYPE_RING, level);

    switch (o_ptr->name2)
    {
    case EGO_RING_DWARVES:
        o_ptr->to_d += 5;
        if (one_in_(3))
            add_flag(o_ptr->art_flags, TR_DEC_DEX);
        if (one_in_(3))
            add_flag(o_ptr->art_flags, TR_WIS);
        if (one_in_(6))
            o_ptr->curse_flags |= TRC_PERMA_CURSE;
        if (one_in_(6))
            add_flag(o_ptr->art_flags, TR_AGGRAVATE);
        if (one_in_(2))
            add_flag(o_ptr->art_flags, TR_RES_DARK);
        if (one_in_(3))
            add_flag(o_ptr->art_flags, TR_RES_DISEN);
        if (one_in_(3))
            add_flag(o_ptr->art_flags, TR_SUST_STR);
        if (one_in_(6))
            one_high_resistance(o_ptr);
        if (one_in_(ACTIVATION_CHANCE))
            effect_add_random(o_ptr, BIAS_PRIESTLY);
        break;
    case EGO_RING_NAZGUL:
        o_ptr->to_d += 6;
        o_ptr->to_h += 6;
        if (one_in_(6))
            o_ptr->curse_flags |= TRC_PERMA_CURSE;
        if (one_in_(66))
            add_flag(o_ptr->art_flags, TR_IM_COLD);
        if (one_in_(6))
            add_flag(o_ptr->art_flags, TR_SLAY_GOOD);
        if (one_in_(6))
            add_flag(o_ptr->art_flags, TR_BRAND_COLD);
        if (one_in_(6))
            add_flag(o_ptr->art_flags, TR_SLAY_HUMAN);
        if (one_in_(6))
            one_high_resistance(o_ptr);
        if (one_in_(ACTIVATION_CHANCE))
            effect_add_random(o_ptr, BIAS_NECROMANTIC);
        break;
    case EGO_RING_COMBAT:
        for (powers = _jewelry_powers(5, level, power); powers > 0; --powers)
        {
            switch (randint1(7))
            {
            case 1:
                if (!have_flag(o_ptr->art_flags, TR_CON))
                {
                    add_flag(o_ptr->art_flags, TR_CON);
                    if (!o_ptr->pval) o_ptr->pval = _jewelry_pval(5, level);
                    break;
                }
            case 2:
                if (!have_flag(o_ptr->art_flags, TR_DEX))
                {
                    add_flag(o_ptr->art_flags, TR_DEX);
                    if (!o_ptr->pval) o_ptr->pval = _jewelry_pval(5, level);
                    break;
                }
            case 3:
                add_flag(o_ptr->art_flags, TR_STR);
                if (!o_ptr->pval) o_ptr->pval = _jewelry_pval(5, level);
                break;
            case 4:
                o_ptr->to_h += randint1(5) + m_bonus(5, level);
                while (one_in_(2) && powers > 0)
                {
                    o_ptr->to_h += randint1(5) + m_bonus(5, level);
                    powers--;
                }
                break;
            case 5:
                o_ptr->to_d += randint1(5) + m_bonus(5, level);
                while (one_in_(2) && powers > 0)
                {
                    o_ptr->to_d += randint1(5) + m_bonus(5, level);
                    powers--;
                }
                break;
            case 6:
                if (abs(power) >= 2 && one_in_(30) && level >= 50)
                {
                    add_flag(o_ptr->art_flags, TR_WEAPONMASTERY);
                    o_ptr->pval = _jewelry_pval(3, level);
                    if (one_in_(30))
                    {
                        switch (randint1(5))
                        {
                        case 1: add_flag(o_ptr->art_flags, TR_BRAND_ACID); break;
                        case 2: add_flag(o_ptr->art_flags, TR_BRAND_COLD); break;
                        case 3: add_flag(o_ptr->art_flags, TR_BRAND_FIRE); break;
                        case 4: add_flag(o_ptr->art_flags, TR_BRAND_ELEC); break;
                        case 5: add_flag(o_ptr->art_flags, TR_BRAND_POIS); break;
                        }
                    }
                    break;
                }
                if (abs(power) >= 2 && one_in_(15) && level >= 50)
                {
                    add_flag(o_ptr->art_flags, TR_BLOWS);
                    o_ptr->pval = _jewelry_pval(3, level);
                    powers = 0;
                }
                else if (one_in_(3))
                {
                    add_flag(o_ptr->art_flags, TR_RES_FEAR);
                    break;
                }
            default:
                o_ptr->to_d += randint1(5) + m_bonus(5, level);
            }
        }
        if (o_ptr->to_h > 25) o_ptr->to_h = 25;
        if (o_ptr->to_d > 20) o_ptr->to_d = 20;
        if (one_in_(ACTIVATION_CHANCE))
            effect_add_random(o_ptr, BIAS_WARRIOR | BIAS_STR);
        break;
    case EGO_RING_ARCHERY:
    {
        int div = 1;
        if (abs(power) >= 2) div++;

        for (powers = _jewelry_powers(5, level, power); powers > 0; --powers)
        {
            switch (randint1(7))
            {
            case 1:
                add_flag(o_ptr->art_flags, TR_DEX);
                if (!o_ptr->pval) o_ptr->pval = _jewelry_pval(5, level);
                break;
            case 2:
                if (_create_level_check(200/div, level - 40))
                    add_flag(o_ptr->art_flags, TR_SPEED);
                else
                    add_flag(o_ptr->art_flags, TR_STEALTH);
                if (!o_ptr->pval) o_ptr->pval = _jewelry_pval(5, level);
                break;
            case 3:
                o_ptr->to_h += randint1(5) + m_bonus(5, level);
                while (one_in_(2) && powers > 0)
                {
                    o_ptr->to_h += randint1(5) + m_bonus(5, level);
                    powers--;
                }
                break;
            case 4:
                o_ptr->to_d += randint1(7) + m_bonus(7, level);
                while (one_in_(2) && powers > 0)
                {
                    o_ptr->to_d += randint1(7) + m_bonus(7, level);
                    powers--;
                }
                break;
            case 5:
                if ( _create_level_check(100/div, level)
                  && (!have_flag(o_ptr->art_flags, TR_XTRA_MIGHT) || one_in_(7) ) )
                {
                    add_flag(o_ptr->art_flags, TR_XTRA_SHOTS);
                    o_ptr->pval = _jewelry_pval(5, level);
                    break;
                }
            case 6:
                if ( _create_level_check(200/div, level)
                  && (!have_flag(o_ptr->art_flags, TR_XTRA_SHOTS) || one_in_(7) ) )
                {
                    add_flag(o_ptr->art_flags, TR_XTRA_MIGHT);
                    o_ptr->pval = _jewelry_pval(5, level);
                    break;
                }
            default:
                o_ptr->to_d += randint1(7) + m_bonus(7, level);
            }
        }
        if (o_ptr->to_h > 30) o_ptr->to_h = 30;
        if (o_ptr->to_d > 40) o_ptr->to_d = 40; /* most players get only one shot ... */
        if (o_ptr->to_d > 20 && have_flag(o_ptr->art_flags, TR_XTRA_SHOTS)) o_ptr->to_d = 20;

        if ( o_ptr->pval > 3
          && (have_flag(o_ptr->art_flags, TR_XTRA_SHOTS) || have_flag(o_ptr->art_flags, TR_XTRA_MIGHT))
          && !one_in_(10) )
        {
            o_ptr->pval = 3;
        }

        if (one_in_(ACTIVATION_CHANCE))
            effect_add_random(o_ptr, BIAS_ARCHER);
        break;
    }
    case EGO_RING_PROTECTION:
        for (powers = _jewelry_powers(5, level, power); powers > 0; --powers)
        {
            switch (randint1(7))
            {
            case 1:
                one_high_resistance(o_ptr);
                if (abs(power) >= 2)
                {
                    do { one_high_resistance(o_ptr); --power; } while(one_in_(3));
                }
                break;
            case 2:
                if (!have_flag(o_ptr->art_flags, TR_FREE_ACT))
                {
                    add_flag(o_ptr->art_flags, TR_FREE_ACT);
                    break;
                }
            case 3:
                if (!have_flag(o_ptr->art_flags, TR_SEE_INVIS))
                {
                    add_flag(o_ptr->art_flags, TR_SEE_INVIS);
                    break;
                }
            case 4:
                if (one_in_(2))
                {
                    add_flag(o_ptr->art_flags, TR_WARNING);
                    if (one_in_(3))
                        one_low_esp(o_ptr);
                    break;
                }
            case 5:
                if (one_in_(2))
                {
                    one_sustain(o_ptr);
                    if (abs(power) >= 2)
                    {
                        do { one_sustain(o_ptr); --power; } while(one_in_(2));
                    }
                    break;
                }
            default:
                o_ptr->to_a += randint1(10);
            }
        }
        if (o_ptr->to_a > 35) o_ptr->to_a = 35;
        if (one_in_(ACTIVATION_CHANCE))
            effect_add_random(o_ptr, BIAS_PROTECTION);
        break;
    case EGO_JEWELRY_ELEMENTAL:
        _ego_create_jewelry_elemental(o_ptr, level, power);
        break;
    case EGO_JEWELRY_DEFENDER:
        _ego_create_jewelry_defender(o_ptr, level, power);
        break;
    case EGO_RING_SPEED:
    {
        /*o_ptr->pval = randint1(5) + m_bonus(5, level);
        while (randint0(100) < 50)
            o_ptr->pval++;*/
        int amt = 5;
        if (level >= 30)
            amt += (MIN(80,level) - 30)/10;
        o_ptr->pval = 1 + m_bonus(amt, level);

        if (randint0(20) < level - 50)
        {
            while (one_in_(2))
                o_ptr->pval++;
        }
        if (level >= 50 && one_in_(ACTIVATION_CHANCE*2))
        {
            if (one_in_(777))
                effect_add(o_ptr, EFFECT_LIGHT_SPEED);
            else if (one_in_(77))
                effect_add(o_ptr, EFFECT_SPEED_HERO);
            else
                effect_add(o_ptr, EFFECT_SPEED);
        }
        break;
    }
    case EGO_RING_WIZARDRY:
        for (powers = _jewelry_powers(4, level, power); powers > 0; --powers)
        {
            switch (randint1(7))
            {
            case 1:
                add_flag(o_ptr->art_flags, TR_INT);
                if (!o_ptr->pval) o_ptr->pval = _jewelry_pval(5, level);
                break;
            case 2:
                add_flag(o_ptr->art_flags, TR_SUST_INT);
                break;
            case 3:
                add_flag(o_ptr->art_flags, TR_SPELL_CAP);
                if (!o_ptr->pval)
                    o_ptr->pval = _jewelry_pval(3, level);
                else if (o_ptr->pval > 3)
                    o_ptr->pval = 3;
                break;
            case 4:
                add_flag(o_ptr->art_flags, TR_EASY_SPELL);
                break;
            case 5:
                if (abs(power) >= 2)
                {
                    add_flag(o_ptr->art_flags, TR_DEC_MANA);
                    break;
                }
                else
                {
                    add_flag(o_ptr->art_flags, TR_LEVITATION);
                    break;
                }
            case 6:
                if (abs(power) >= 2 && one_in_(30))
                {
                    add_flag(o_ptr->art_flags, TR_SPELL_POWER);
                    add_flag(o_ptr->art_flags, TR_DEC_STR);
                    add_flag(o_ptr->art_flags, TR_DEC_DEX);
                    add_flag(o_ptr->art_flags, TR_DEC_CON);
                    o_ptr->pval = _jewelry_pval(2, level);
                }
                else
                {
                    o_ptr->to_d += randint1(5) + m_bonus(5, level);
                    while (one_in_(2) && powers > 0)
                    {
                        o_ptr->to_d += randint1(5) + m_bonus(5, level);
                        powers--;
                    }
                }
                break;
            default:
                if (abs(power) >= 2 && one_in_(15))
                    add_flag(o_ptr->art_flags, TR_TELEPATHY);
                else
                    one_low_esp(o_ptr);
            }
        }
        if (o_ptr->to_d > 20)
            o_ptr->to_d = 20;
        if (one_in_(ACTIVATION_CHANCE))
            effect_add_random(o_ptr, BIAS_MAGE);
        break;
    }

    _finalize_jewelry(o_ptr);

    /* Be sure to cursify later! */
    if (power == -1)
        power--;

    ego_finalize(o_ptr, level, power, mode);
}

static void _create_amulet_aux(object_type *o_ptr, int level, int power, int mode)
{
    int powers = 0;

    o_ptr->name2 = ego_choose_type(EGO_TYPE_AMULET, level);

    switch (o_ptr->name2)
    {
    case EGO_AMULET_MAGI:
        add_flag(o_ptr->art_flags, TR_SEARCH);
        for (powers = _jewelry_powers(5, level, power); powers > 0; --powers)
        {
            switch (randint1(7))
            {
            case 1:
                add_flag(o_ptr->art_flags, TR_FREE_ACT);
                add_flag(o_ptr->art_flags, TR_SEE_INVIS);
                break;
            case 2:
                add_flag(o_ptr->art_flags, TR_SUST_INT);
                break;
            case 3:
                add_flag(o_ptr->art_flags, TR_EASY_SPELL);
                break;
            case 4:
                if (abs(power) >= 2 && one_in_(2))
                    add_flag(o_ptr->art_flags, TR_TELEPATHY);
                else
                    one_low_esp(o_ptr);
                break;
            case 5:
                if (abs(power) >= 2)
                {
                    add_flag(o_ptr->art_flags, TR_DEC_MANA);
                    break;
                }
                else if (one_in_(2))
                {
                    add_flag(o_ptr->art_flags, TR_MAGIC_MASTERY);
                    if (!o_ptr->pval) o_ptr->pval = _jewelry_pval(5, level);
                    break;
                }
            case 6:
                if (abs(power) >= 2 && one_in_(15))
                {
                    add_flag(o_ptr->art_flags, TR_SPELL_POWER);
                    add_flag(o_ptr->art_flags, TR_DEC_STR);
                    add_flag(o_ptr->art_flags, TR_DEC_DEX);
                    add_flag(o_ptr->art_flags, TR_DEC_CON);
                    o_ptr->pval = _jewelry_pval(2, level);
                }
                else
                {
                    o_ptr->to_d += randint1(5) + m_bonus(5, level);
                    while (one_in_(2) && powers > 0)
                    {
                        o_ptr->to_d += randint1(5) + m_bonus(5, level);
                        powers--;
                    }
                }
                break;
            default:
                add_flag(o_ptr->art_flags, TR_INT);
                if (!o_ptr->pval) o_ptr->pval = _jewelry_pval(5, level);
            }
        }
        if (!o_ptr->pval) o_ptr->pval = randint1(8); /* Searching */
        if (o_ptr->to_d > 20)
            o_ptr->to_d = 20;
        if (one_in_(ACTIVATION_CHANCE))
            effect_add_random(o_ptr, BIAS_MAGE);
        break;
    case EGO_AMULET_DEVOTION:
        for (powers = _jewelry_powers(5, level, power); powers > 0; --powers)
        {
            switch (randint1(7))
            {
            case 1:
                add_flag(o_ptr->art_flags, TR_CHR);
                if (!o_ptr->pval) o_ptr->pval = _jewelry_pval(5, level);
                break;
            case 2:
                add_flag(o_ptr->art_flags, TR_REFLECT);
                break;
            case 3:
                if (abs(power) >= 2 && one_in_(2) && level >= 30)
                {
                    add_flag(o_ptr->art_flags, TR_SPELL_CAP);
                    o_ptr->pval = _jewelry_pval(3, level);
                }
                else
                {
                    add_flag(o_ptr->art_flags, TR_HOLD_LIFE);
                    if (one_in_(2))
                        add_flag(o_ptr->art_flags, TR_FREE_ACT);
                    if (one_in_(2))
                        add_flag(o_ptr->art_flags, TR_SEE_INVIS);
                }
                break;
            case 4:
                one_high_resistance(o_ptr);
                break;
            case 5:
                if (abs(power) >= 2 && one_in_(2) && level >= 30)
                {
                    do { one_high_resistance(o_ptr); } while (one_in_(3));
                    break;
                }
            default:
                add_flag(o_ptr->art_flags, TR_WIS);
                if (!o_ptr->pval) o_ptr->pval = _jewelry_pval(5, level);
            }
        }
        if (one_in_(ACTIVATION_CHANCE))
            effect_add_random(o_ptr, BIAS_PRIESTLY);
        break;
    case EGO_AMULET_TRICKERY:
        add_flag(o_ptr->art_flags, TR_SEARCH);
        for (powers = _jewelry_powers(5, level, power); powers > 0; --powers)
        {
            switch (randint1(7))
            {
            case 1:
                add_flag(o_ptr->art_flags, TR_DEX);
                if (!o_ptr->pval) o_ptr->pval = _jewelry_pval(5, level);
                break;
            case 2:
                add_flag(o_ptr->art_flags, TR_SUST_DEX);
                break;
            case 3:
                if (one_in_(2))
                    add_flag(o_ptr->art_flags, TR_RES_POIS);
                else
                    add_flag(o_ptr->art_flags, TR_RES_DARK);
                break;
            case 4:
                if (one_in_(2))
                    add_flag(o_ptr->art_flags, TR_RES_NEXUS);
                else
                    add_flag(o_ptr->art_flags, TR_RES_CONF);
                break;
            case 5:
                if (abs(power) >= 2 && one_in_(2) && level >= 50)
                {
                    add_flag(o_ptr->art_flags, TR_TELEPATHY);
                    break;
                }
            case 6:
                if (abs(power) >= 2 && one_in_(2) && level >= 50)
                {
                    add_flag(o_ptr->art_flags, TR_SPEED);
                    o_ptr->pval = _jewelry_pval(3, level);
                    break;
                }
            default:
                add_flag(o_ptr->art_flags, TR_STEALTH);
            }
        }
        if (!o_ptr->pval) o_ptr->pval = randint1(5); /* Searching & Stealth */
        if (one_in_(ACTIVATION_CHANCE))
            effect_add_random(o_ptr, BIAS_ROGUE);
        break;
    case EGO_AMULET_HERO:
        o_ptr->to_a = randint1(5) + m_bonus(5, level);
        o_ptr->to_h = randint1(3) + m_bonus(5, level);
        o_ptr->to_d = randint1(3) + m_bonus(5, level);
        if (one_in_(3)) add_flag(o_ptr->art_flags, TR_SLOW_DIGEST);
        if (one_in_(3)) add_flag(o_ptr->art_flags, TR_SUST_CON);
        if (one_in_(3)) add_flag(o_ptr->art_flags, TR_SUST_STR);
        if (one_in_(3)) add_flag(o_ptr->art_flags, TR_SUST_DEX);
        if (one_in_(ACTIVATION_CHANCE))
            effect_add_random(o_ptr, BIAS_WARRIOR);
        break;
    case EGO_AMULET_DWARVEN:
        add_flag(o_ptr->art_flags, TR_INFRA);
        if (one_in_(2)) add_flag(o_ptr->art_flags, TR_LITE);
        for (powers = _jewelry_powers(5, level, power); powers > 0; --powers)
        {
            switch (randint1(9))
            {
            case 1:
                add_flag(o_ptr->art_flags, TR_STR);
                if (!o_ptr->pval) o_ptr->pval = _jewelry_pval(4, level);
                break;
            case 2:
                if (one_in_(3))
                    add_flag(o_ptr->art_flags, TR_DEC_DEX);
                else
                    add_flag(o_ptr->art_flags, TR_DEC_STEALTH);

                if (one_in_(5))
                    add_flag(o_ptr->art_flags, TR_WIS);

                if (!o_ptr->pval) o_ptr->pval = _jewelry_pval(4, level);
                break;
            case 3:
                add_flag(o_ptr->art_flags, TR_CON);
                if (!o_ptr->pval) o_ptr->pval = _jewelry_pval(4, level);
                break;
            case 4:
                add_flag(o_ptr->art_flags, TR_RES_BLIND);
                break;
            case 5:
                add_flag(o_ptr->art_flags, TR_RES_DARK);
                break;
            case 6:
                add_flag(o_ptr->art_flags, TR_RES_DISEN);
                break;
            case 7:
                add_flag(o_ptr->art_flags, TR_FREE_ACT);
                break;
            default:
                add_flag(o_ptr->art_flags, TR_REGEN);
            }
        }
        if (!o_ptr->pval) o_ptr->pval = 2 + randint1(6); /* Infravision */
        break;
    case EGO_AMULET_BARBARIAN:
        for (powers = _jewelry_powers(5, level, power); powers > 0; --powers)
        {
            switch (randint1(6))
            {
            case 1:
                add_flag(o_ptr->art_flags, TR_STR);
                if (!o_ptr->pval) o_ptr->pval = _jewelry_pval(4, level);
                break;
            case 2:
                add_flag(o_ptr->art_flags, TR_DEX);
                if (!o_ptr->pval) o_ptr->pval = _jewelry_pval(4, level);
                break;
            case 3:
                if (one_in_(3))
                    add_flag(o_ptr->art_flags, TR_FREE_ACT);
                else
                {
                    add_flag(o_ptr->art_flags, TR_NO_MAGIC);
                    if (abs(power) >= 2 && one_in_(10) && level >= 70)
                    {
                        add_flag(o_ptr->art_flags, TR_MAGIC_RESISTANCE);
                        o_ptr->pval = _jewelry_pval(3, level);
                    }
                }
                break;
            case 4:
                if (abs(power) >= 2 && one_in_(10) && level >= 70)
                    add_flag(o_ptr->art_flags, TR_NO_SUMMON);
                else if (one_in_(6))
                    add_flag(o_ptr->art_flags, TR_NO_TELE);
                else
                    add_flag(o_ptr->art_flags, TR_RES_FEAR);
                break;
            case 5:
                add_flag(o_ptr->art_flags, TR_DEC_INT);
                if (!o_ptr->pval) o_ptr->pval = _jewelry_pval(4, level);
                break;
            case 6:
                o_ptr->to_a += randint1(5) + m_bonus(5, level);
                break;
            }
        }
        if (o_ptr->to_a > 15) o_ptr->to_a = 15;
        if (one_in_(ACTIVATION_CHANCE*2))
            effect_add(o_ptr, EFFECT_BERSERK);
        break;
    case EGO_AMULET_SACRED:
        add_flag(o_ptr->art_flags, TR_BLESSED);
        o_ptr->to_a = 5;
        if (one_in_(2)) add_flag(o_ptr->art_flags, TR_LITE);
        for (powers = _jewelry_powers(5, level, power); powers > 0; --powers)
        {
            switch (randint1(8))
            {
            case 1:
                add_flag(o_ptr->art_flags, TR_STR);
                if (!o_ptr->pval) o_ptr->pval = _jewelry_pval(4, level);
                break;
            case 2:
                add_flag(o_ptr->art_flags, TR_WIS);
                if (!o_ptr->pval) o_ptr->pval = _jewelry_pval(4, level);
                break;
            case 3:
                add_flag(o_ptr->art_flags, TR_FREE_ACT);
                if (one_in_(2))
                    add_flag(o_ptr->art_flags, TR_SEE_INVIS);
                break;
            case 4:
                if (abs(power) >= 2 && one_in_(10) && level >= 50)
                    add_flag(o_ptr->art_flags, TR_REFLECT);
                else if (one_in_(5))
                    add_flag(o_ptr->art_flags, TR_RES_CHAOS);
                else if (one_in_(3))
                    add_flag(o_ptr->art_flags, TR_RES_CONF);
                else if (one_in_(3))
                    add_flag(o_ptr->art_flags, TR_RES_NETHER);
                else
                    add_flag(o_ptr->art_flags, TR_RES_FEAR);
                break;
            case 5:
                if (abs(power) >= 2 && one_in_(20) && level >= 70)
                {
                    add_flag(o_ptr->art_flags, TR_SPEED);
                    o_ptr->pval = _jewelry_pval(3, level);
                }
                else if (one_in_(7) && level >= 50)
                {
                    add_flag(o_ptr->art_flags, TR_LIFE);
                    if (!o_ptr->pval) o_ptr->pval = _jewelry_pval(4, level);
                }
                else if (one_in_(2))
                    add_flag(o_ptr->art_flags, TR_REGEN);
                else
                    add_flag(o_ptr->art_flags, TR_HOLD_LIFE);
                break;
            default:
                o_ptr->to_a += randint1(5) + m_bonus(5, level);
            }
        }
        if (o_ptr->to_a > 20) o_ptr->to_a = 20;
        if (one_in_(ACTIVATION_CHANCE))
            effect_add_random(o_ptr, BIAS_LAW);
        break;
    case EGO_AMULET_HELL:
        o_ptr->curse_flags |= TRC_CURSED;
        o_ptr->to_a = -5;
        for (powers = _jewelry_powers(5, level, power); powers > 0; --powers)
        {
            switch (randint1(7))
            {
            case 1:
                if (one_in_(3))
                {
                    add_flag(o_ptr->art_flags, TR_AGGRAVATE);
                    one_demon_resistance(o_ptr);
                }
                else
                {
                    add_flag(o_ptr->art_flags, TR_DEC_STEALTH);
                    add_flag(o_ptr->art_flags, TR_DEC_WIS);
                    if (!o_ptr->pval) o_ptr->pval = randint1(7);
                }
                break;
            case 2:
                o_ptr->to_a -= randint1(5) + m_bonus(5, level);
                one_demon_resistance(o_ptr);
                break;
            case 3:
                add_flag(o_ptr->art_flags, TR_FREE_ACT);
                if (one_in_(2))
                    add_flag(o_ptr->art_flags, TR_SEE_INVIS);
                if (one_in_(6))
                {
                    add_flag(o_ptr->art_flags, TR_VULN_COLD);
                    o_ptr->to_h += randint1(3);
                    o_ptr->to_d += randint1(5);
                    o_ptr->to_a -= randint1(5);
                    one_demon_resistance(o_ptr);
                }
                break;
            case 4:
                o_ptr->to_h += randint1(5) + m_bonus(5, level);
                break;
            case 5:
                o_ptr->to_d += randint1(5) + m_bonus(5, level);
                if (one_in_(6))
                {
                    add_flag(o_ptr->art_flags, TR_DRAIN_EXP);
                    one_demon_resistance(o_ptr);
                }
                break;
            case 6:
                if (abs(power) >= 2 && one_in_(66) && level >= 66)
                {
                    add_flag(o_ptr->art_flags, TR_TY_CURSE);
                    add_flag(o_ptr->art_flags, TR_IM_FIRE);
                    o_ptr->to_h += randint1(6);
                    o_ptr->to_d += randint1(6);
                    o_ptr->to_a -= randint1(20);
                    break;
                }
                else if (one_in_(3))
                {
                    add_flag(o_ptr->art_flags, TR_DEC_SPEED);
                    o_ptr->pval = randint1(3);
                    o_ptr->to_h += randint1(3);
                    o_ptr->to_d += randint1(5);
                    o_ptr->to_a -= randint1(5);
                    one_demon_resistance(o_ptr);
                    break;
                }
            default:
                o_ptr->to_h += randint1(3);
                o_ptr->to_d += randint1(5);
            }
        }
        if (o_ptr->to_a < -20) o_ptr->to_a = -20;
        if (o_ptr->to_h > 20) o_ptr->to_h = 20;
        if (o_ptr->to_d > 16)
        {
            add_flag(o_ptr->art_flags, TR_AGGRAVATE);
            o_ptr->to_d = 16;
        }
        if (one_in_(ACTIVATION_CHANCE))
            effect_add_random(o_ptr, BIAS_DEMON);
        break;
    case EGO_JEWELRY_ELEMENTAL:
        _ego_create_jewelry_elemental(o_ptr, level, power);
        break;
    case EGO_JEWELRY_DEFENDER:
        _ego_create_jewelry_defender(o_ptr, level, power);
        break;
    }

    _finalize_jewelry(o_ptr);

    /* Be sure to cursify later! */
    if (power == -1)
        power--;

    ego_finalize(o_ptr, level, power, mode);
}

/*************************************************************************
 * Devices
 *************************************************************************/
bool obj_create_device(object_type *o_ptr, int level, int power, int mode)
{
    /* Create the device and pick the effect. This can fail if, for example,
       mode is AM_GOOD and we are too shallow for any of the good effects. */
    if (!device_init(o_ptr, level, mode))
        return FALSE;

    if (abs(power) > 1)
    {
        bool done = FALSE;
        u32b flgs[TR_FLAG_ARRAY_SIZE];

        obj_flags(o_ptr, flgs);

        while (!done)
        {
            o_ptr->name2 = ego_choose_type(EGO_TYPE_DEVICE, level);
            done = TRUE;

            if ( o_ptr->name2 == EGO_DEVICE_RESISTANCE
              && (have_flag(flgs, TR_IGNORE_ACID) || o_ptr->tval == TV_ROD) )
            {
                done = FALSE;
            }
        }

        switch (o_ptr->name2)
        {
        case EGO_DEVICE_CAPACITY:
            o_ptr->pval = 1 + m_bonus(4, level);
            o_ptr->xtra4 += o_ptr->xtra4 * o_ptr->pval * 10 / 100;
            break;
        case EGO_DEVICE_SIMPLICITY:
        case EGO_DEVICE_POWER:
        case EGO_DEVICE_REGENERATION:
        case EGO_DEVICE_QUICKNESS:
            o_ptr->pval = 1 + m_bonus(4, level);
            break;
        }
    }

    if (power < 0)
        o_ptr->curse_flags |= TRC_CURSED;

    return TRUE;
}

/*************************************************************************
 * Weapons
 *************************************************************************/
void ego_weapon_adjust_weight(object_type *o_ptr)
{
    /* Experimental: Maulers need heavy weapons!
        Anything that dice boosts gets heavier. */
    if (object_is_melee_weapon(o_ptr) && p_ptr->pclass == CLASS_MAULER)
    {
    object_kind *k_ptr = &k_info[o_ptr->k_idx];
    int          dice = o_ptr->dd * o_ptr->ds;
    int          orig = k_ptr->dd * k_ptr->ds;

        if (dice > orig)
        {
            int wgt = o_ptr->weight;
            int xtra = k_ptr->weight;
            int mult = (dice - orig) * 100 / orig;

            while (mult >= 100)
            {
                xtra = xtra * 3 / 4;
                wgt += xtra;
                mult -= 100;
            }
            if (mult > 0)
            {
                xtra = xtra * 3 / 4;
                wgt += xtra * mult / 100;
            }

            o_ptr->weight = wgt;

            /*  a Bo Staff (1d11) ... 16.0 lbs
                a Bo Staff (2d12) ... 29.6 lbs
                a Bo Staff (3d12) ... 38.8 lbs
                a Bo Staff (4d12) ... 45.5 lbs
                a Bo Staff (5d12) ... 50.3 lbs
                a Bo Staff (6d12) ... 53.8 lbs
                a Bo Staff (7d12) ... 56.3 lbs

                a Heavy Lance (4d8) ... 40.0 lbs
                a Heavy Lance (5d8) ... 47.5 lbs
                a Heavy Lance (6d8) ... 55.0 lbs
                a Heavy Lance (8d8) ... 70.0 lbs
                a Heavy Lance (8d9) ... 75.6 lbs

                a Dagger (1d4) ... 1.2 lbs
                a Dagger (2d4) ... 2.1 lbs
                a Dagger (3d4) ... 2.7 lbs
                a Dagger (7d5) ... 3.7 lbs
            */
        }
    }
}
static void _ego_create_harp(object_type *o_ptr, int level)
{
    o_ptr->name2 = ego_choose_type(EGO_TYPE_HARP, level);
}
static void _ego_create_bow(object_type *o_ptr, int level)
{
    bool done = FALSE;
    while (!done)
    {
        o_ptr->name2 = ego_choose_type(EGO_TYPE_BOW, level);
        done = TRUE;

        switch (o_ptr->name2)
        {
        case EGO_BOW_VELOCITY:
            o_ptr->mult  += 25;
            break;
        case EGO_BOW_EXTRA_MIGHT:
            o_ptr->mult  += 25 + m_bonus(15, level) * 5;
            break;
        case EGO_BOW_LOTHLORIEN:
            if (o_ptr->sval != SV_LONG_BOW)
                done = FALSE;
            else
            {
                o_ptr->mult  += 25 + m_bonus(17, level) * 5;

                if (one_in_(3))
                    add_flag(o_ptr->art_flags, TR_XTRA_SHOTS);
                else
                    one_high_resistance(o_ptr);
            }
            break;
        case EGO_BOW_BUCKLAND:
            if (o_ptr->sval != SV_SLING)
                done = FALSE;
            else
            {
                if (one_in_(3))
                    o_ptr->mult  += 25 + m_bonus(15, level) * 5;
                else
                    one_high_resistance(o_ptr);
            }
            break;
        case EGO_BOW_HARADRIM:
            if (o_ptr->sval != SV_HEAVY_XBOW)
                done = FALSE;
            else
            {
                o_ptr->mult  += 25 + m_bonus(20, level) * 5;
                if (one_in_(3))
                {
                    add_flag(o_ptr->art_flags, TR_XTRA_SHOTS);
                    add_flag(o_ptr->art_flags, TR_DEC_SPEED);
                }
                else
                    one_high_resistance(o_ptr);
            }
            break;
        }
    }
}
static void _ego_create_digger(object_type *o_ptr, int level)
{
    bool done = FALSE;
    while (!done)
    {
        o_ptr->name2 = ego_choose_type(EGO_TYPE_DIGGER, level);
        done = TRUE;
        switch (o_ptr->name2)
        {
        case EGO_DIGGER_DISSOLVING:
            o_ptr->dd += 1;
            break;
        case EGO_DIGGER_DISRUPTION:
            if (o_ptr->sval != SV_MATTOCK)
                done = FALSE;
            else
                o_ptr->dd += 2;
            break;
        }
    }
}
typedef struct {
    int slay_flag;
    int rarity;
    int kill_flag;
    int esp_flag;
} _slaying_info_t, *_slaying_info_ptr;
static _slaying_info_t _slaying_info[] = {
    { TR_SLAY_ORC,     2, TR_KILL_ORC,    TR_ESP_ORC},
    { TR_SLAY_TROLL,   2, TR_KILL_TROLL,  TR_ESP_TROLL},
    { TR_SLAY_GIANT,   2, TR_KILL_GIANT,  TR_ESP_GIANT},
    { TR_SLAY_DRAGON,  3, TR_KILL_DRAGON, TR_ESP_DRAGON},
    { TR_SLAY_DEMON,   3, TR_KILL_DEMON,  TR_ESP_DEMON},
    { TR_SLAY_UNDEAD,  3, TR_KILL_UNDEAD, TR_ESP_UNDEAD},
    { TR_SLAY_ANIMAL,  2, TR_KILL_ANIMAL, TR_ESP_ANIMAL},
    { TR_SLAY_HUMAN,   3, TR_KILL_HUMAN,  TR_ESP_HUMAN},
    { TR_SLAY_EVIL,    5, TR_KILL_EVIL,   TR_ESP_EVIL},
    { TR_SLAY_GOOD,    5, TR_INVALID,     TR_ESP_GOOD},
    { TR_SLAY_LIVING, 20, TR_INVALID,     TR_INVALID},
    { TR_INVALID },
};
static int _choose_slaying_info(void)
{
    int i, n;
    int tot = 0;

    for (i = 0; ; i++)
    {
        if (_slaying_info[i].slay_flag == TR_INVALID) break;
        if (!_slaying_info[i].rarity) continue;

        tot += MAX(255 / _slaying_info[i].rarity, 1);
    }

    if (!tot) return -1;
    n = randint1(tot);

    for (i = 0; ; i++)
    {
        if (_slaying_info[i].slay_flag == TR_INVALID) break;
        if (!_slaying_info[i].rarity) continue;

        n -= MAX(255 / _slaying_info[i].rarity, 1);
        if (n <= 0) return i;
    }
    return -1;
}
static void _ego_create_weapon_slaying(object_type *o_ptr, int level)
{
    int rolls = 1 + m_bonus(4, level);
    int i;

    assert(o_ptr->name2 == EGO_WEAPON_SLAYING);

    if (one_in_(GREAT_OBJ))
        rolls *= 2;

    for (i = 0; i < rolls; i++)
    {
        int               j = _choose_slaying_info();
        _slaying_info_ptr info;
        if (j == -1) continue;
        info = &_slaying_info[j];

        if (info->kill_flag != TR_INVALID && one_in_(info->rarity*info->rarity*info->rarity))
        {
            add_flag(o_ptr->art_flags, info->kill_flag);
            if (info->esp_flag != TR_INVALID)
                add_flag(o_ptr->art_flags, info->esp_flag);
        }
        else
        {
            add_flag(o_ptr->art_flags, info->slay_flag);
            if (info->esp_flag != TR_INVALID && one_in_(6))
                add_flag(o_ptr->art_flags, info->esp_flag);
        }
    }
}
static void _ego_create_weapon_armageddon(object_type *o_ptr, int level)
{
    int odds = o_ptr->dd * o_ptr->ds / 2;

    assert(o_ptr->name2 == EGO_WEAPON_ARMAGEDDON);
    if (odds < 3) odds = 3;
    if (one_in_(odds)) /* double damage */
    {
        o_ptr->dd *= 2;

        /* Look alikes to keep players happy */
        if (object_is_(o_ptr, TV_SWORD, SV_LONG_SWORD) && one_in_(2))
            o_ptr->dd = 5; /* Vorpal Blade */
        if (object_is_(o_ptr, TV_SWORD, SV_KATANA) && one_in_(3))
        {
            o_ptr->dd = 8; /* Aglarang */
            o_ptr->ds = 4;
            if (one_in_(100))
            {
                o_ptr->dd = 10;
                o_ptr->ds = 5; /* Muramasa */
            }
        }
        if (object_is_(o_ptr, TV_HAFTED, SV_WAR_HAMMER) && one_in_(2))
            o_ptr->dd = 9; /* Aule */
    }
    else
    {
        do
        {
            o_ptr->dd++;
        }
        while (one_in_(o_ptr->dd));

        do
        {
            o_ptr->ds++;
        }
        while (one_in_(o_ptr->ds));
    }

    if (one_in_(5))
    {
        switch (randint1(5))
        {
        case 1: add_flag(o_ptr->art_flags, TR_BRAND_ELEC); break;
        case 2: add_flag(o_ptr->art_flags, TR_BRAND_FIRE); break;
        case 3: add_flag(o_ptr->art_flags, TR_BRAND_COLD); break;
        case 4: add_flag(o_ptr->art_flags, TR_BRAND_ACID); break;
        default: add_flag(o_ptr->art_flags, TR_BRAND_POIS); break;
        }
    }

    if (o_ptr->tval == TV_SWORD)
    {
        if (one_in_(3))
            add_flag(o_ptr->art_flags, TR_VORPAL);
        else if (one_in_(777))
            add_flag(o_ptr->art_flags, TR_VORPAL2);
    }

    if (o_ptr->tval == TV_HAFTED)
    {
        if (one_in_(7))
            add_flag(o_ptr->art_flags, TR_IMPACT);
        else if (one_in_(7))
            add_flag(o_ptr->art_flags, TR_STUN);
    }

    if (one_in_(666))
        add_flag(o_ptr->art_flags, TR_FORCE_WEAPON);

}
static void _ego_create_weapon_craft(object_type *o_ptr, int level)
{
    int rolls = 1 + m_bonus(4, level);
    int i;

    assert(o_ptr->name2 == EGO_WEAPON_CRAFT);


    if (one_in_(GREAT_OBJ))
        rolls *= 2;

    for (i = 0; i < rolls; i++)
    {
        int res_flag = TR_INVALID, bias = 0;
        switch (randint1(5))
        {
        case 1:
            add_flag(o_ptr->art_flags, TR_BRAND_ACID);
            res_flag = TR_RES_ACID;
            bias = BIAS_ACID;
            break;
        case 2:
            add_flag(o_ptr->art_flags, TR_BRAND_ELEC);
            res_flag = TR_RES_ELEC;
            bias = BIAS_ELEC;
            break;
        case 3:
            add_flag(o_ptr->art_flags, TR_BRAND_FIRE);
            res_flag = TR_RES_FIRE;
            bias = BIAS_FIRE;
            break;
        case 4:
            add_flag(o_ptr->art_flags, TR_BRAND_COLD);
            res_flag = TR_RES_COLD;
            bias = BIAS_COLD;
            break;
        case 5:
            add_flag(o_ptr->art_flags, TR_BRAND_POIS);
            res_flag = TR_RES_POIS;
            bias = BIAS_POIS;
            break;
        }

        /* Mimic old boring brand flavors ... */
        if (i == 0 && one_in_(2))
        {
            add_flag(o_ptr->art_flags, res_flag);
            if (one_in_(ACTIVATION_CHANCE))
                effect_add_random(o_ptr, bias);
            break;
        }

        if (one_in_(3))
            add_flag(o_ptr->art_flags, res_flag);
    }
    if (one_in_(6) && level > 60)
        add_flag(o_ptr->art_flags, TR_FORCE_WEAPON);
}
static void _ego_create_weapon_defender(object_type *o_ptr, int level)
{
    assert(o_ptr->name2 == EGO_WEAPON_DEFENDER);

    o_ptr->to_a = 5;
    if (one_in_(4))
    {
        int i;
        int ct = 2;

        while (one_in_(3))
            ct++;

        for (i = 0; i < ct; i++)
            one_high_resistance(o_ptr);
    }
    else
    {
        int i;
        int ct = 4;

        while (one_in_(2))
            ct++;

        for (i = 0; i < ct; i++)
            one_ele_resistance(o_ptr);
    }
    if (one_in_(3))
        add_flag(o_ptr->art_flags, TR_WARNING);
    if (one_in_(3))
        add_flag(o_ptr->art_flags, TR_LEVITATION);
    if (one_in_(7))
        add_flag(o_ptr->art_flags, TR_REGEN);
}
static void _ego_create_weapon(object_type *o_ptr, int level)
{
    bool done = FALSE;
    while (!done)
    {
        o_ptr->name2 = ego_choose_type(EGO_TYPE_WEAPON, level);
        done = TRUE;
        switch (o_ptr->name2)
        {
        case EGO_WEAPON_SLAYING:
            _ego_create_weapon_slaying(o_ptr, level);
            break;
        case EGO_WEAPON_SHARPNESS:
            if (o_ptr->tval != TV_SWORD)
                done = FALSE;
            else
            {
                o_ptr->pval = m_bonus(5, level) + 1;
                if (one_in_(2))
                {
                    do
                    {
                        o_ptr->dd++;
                    }
                    while (one_in_(o_ptr->dd));
                }
                if (one_in_(7))
                    add_flag(o_ptr->art_flags, TR_VORPAL2);
                else
                    add_flag(o_ptr->art_flags, TR_VORPAL);
            }
            break;

        case EGO_WEAPON_ARCANE:
            if (o_ptr->tval != TV_HAFTED || o_ptr->sval != SV_WIZSTAFF)
                done = FALSE;
            else
            {
                o_ptr->pval = randint1(2);
                if (one_in_(30))
                    o_ptr->pval++;
                o_ptr->to_h = -10;
                o_ptr->to_d = -10;
                if (one_in_(ACTIVATION_CHANCE))
                    effect_add_random(o_ptr, BIAS_MAGE);
            }
            break;
        case EGO_WEAPON_ARMAGEDDON:
            _ego_create_weapon_armageddon(o_ptr, level);
            break;
        case EGO_WEAPON_CHAOS:
            if (one_in_(ACTIVATION_CHANCE))
                effect_add_random(o_ptr, BIAS_CHAOS);
            break;
        case EGO_WEAPON_CRAFT:
            _ego_create_weapon_craft(o_ptr, level);
            break;
        case EGO_WEAPON_CRUSADE:
            if (one_in_(4) && level > 40)
                add_flag(o_ptr->art_flags, TR_BLOWS);
            else if (one_in_(777) && level > 80)
                add_flag(o_ptr->art_flags, TR_FORCE_WEAPON);
            if (one_in_(ACTIVATION_CHANCE))
                effect_add_random(o_ptr, BIAS_PRIESTLY);
            break;
        case EGO_WEAPON_DAEMON:
            if (one_in_(3))
                add_flag(o_ptr->art_flags, TR_SLAY_GOOD);
            if (one_in_(5))
                add_flag(o_ptr->art_flags, TR_AGGRAVATE);
            else
                add_flag(o_ptr->art_flags, TR_DEC_STEALTH);
            if (p_ptr->pclass == CLASS_PRIEST && (p_ptr->realm1 == REALM_DAEMON || p_ptr->realm2 == REALM_DAEMON))
                add_flag(o_ptr->art_flags, TR_BLESSED);
            break;
        case EGO_WEAPON_DEATH:
            if (one_in_(6))
                add_flag(o_ptr->art_flags, TR_VULN_LITE);
            if (one_in_(3))
                add_flag(o_ptr->art_flags, TR_SLAY_GOOD);
            if (one_in_(3))
                add_flag(o_ptr->art_flags, TR_RES_NETHER);
            if (one_in_(5))
                add_flag(o_ptr->art_flags, TR_SLAY_HUMAN);
            else if (one_in_(13))
            {
                add_flag(o_ptr->art_flags, TR_SLAY_LIVING);
                o_ptr->dd++;
                o_ptr->curse_flags |= TRC_CURSED;
                o_ptr->curse_flags |= get_curse(2, o_ptr);
            }
            if (one_in_(ACTIVATION_CHANCE))
                effect_add_random(o_ptr, BIAS_NECROMANTIC);
            if (p_ptr->pclass == CLASS_PRIEST && (p_ptr->realm1 == REALM_DEATH || p_ptr->realm2 == REALM_DEATH))
                add_flag(o_ptr->art_flags, TR_BLESSED);
            break;
        case EGO_WEAPON_NATURE:
            if (one_in_(5))
                add_flag(o_ptr->art_flags, TR_KILL_ANIMAL);
            if (one_in_(3))
                add_flag(o_ptr->art_flags, TR_BRAND_ELEC);
            if (one_in_(3))
            {
                add_flag(o_ptr->art_flags, TR_BRAND_FIRE);
                if (object_is_(o_ptr, TV_HAFTED, SV_WHIP))
                    o_ptr->dd++;
            }
            if (one_in_(3))
                add_flag(o_ptr->art_flags, TR_BRAND_COLD);
            if (one_in_(3))
                add_flag(o_ptr->art_flags, TR_RES_ELEC);
            if (one_in_(3))
                add_flag(o_ptr->art_flags, TR_RES_FIRE);
            if (one_in_(3))
                add_flag(o_ptr->art_flags, TR_RES_COLD);
            if (one_in_(ACTIVATION_CHANCE))
                effect_add_random(o_ptr, BIAS_RANGER);
            break;
        case EGO_WEAPON_TRUMP:
            if (one_in_(3))
                add_flag(o_ptr->art_flags, TR_CHR);
            if (one_in_(5))
                add_flag(o_ptr->art_flags, TR_SLAY_DEMON);
            if (one_in_(7))
                one_ability(o_ptr);
            break;
        case EGO_WEAPON_WILD:
            o_ptr->ds = o_ptr->dd * o_ptr->ds;
            o_ptr->dd = 1;
            break;
        case EGO_WEAPON_ORDER:
            o_ptr->dd = o_ptr->dd * o_ptr->ds;
            o_ptr->ds = 1;
            break;
        case EGO_WEAPON_DEFENDER:
            _ego_create_weapon_defender(o_ptr, level);
            break;
        case EGO_WEAPON_WESTERNESSE:
            if (one_in_(3))
                add_flag(o_ptr->art_flags, TR_RES_FEAR);
            break;
        case EGO_WEAPON_MORGUL:
            if (p_ptr->pclass == CLASS_PRIEST && (p_ptr->realm1 == REALM_DEATH || p_ptr->realm2 == REALM_DEATH))
                add_flag(o_ptr->art_flags, TR_BLESSED);
            break;
        case EGO_WEAPON_PATTERN:
            if (one_in_(3))
                add_flag(o_ptr->art_flags, TR_HOLD_LIFE);
            if (one_in_(3))
                add_flag(o_ptr->art_flags, TR_DEX);
            if (one_in_(5))
                add_flag(o_ptr->art_flags, TR_RES_FEAR);
            break;
        case EGO_WEAPON_NOLDOR:
            if ( o_ptr->tval != TV_SWORD
              || o_ptr->sval == SV_BLADE_OF_CHAOS
              || o_ptr->dd * o_ptr->ds < 10 )
            {
                done = FALSE;
            }
            else
            {
                o_ptr->dd += 1;
            }
            break;
        case EGO_WEAPON_JOUSTING:
            if ( !object_is_(o_ptr, TV_POLEARM, SV_LANCE)
              && !object_is_(o_ptr, TV_POLEARM, SV_HEAVY_LANCE) )
            {
                done = FALSE;
            }
            else
            {
                while (one_in_(o_ptr->dd * 3)) { o_ptr->dd++; }
                if (one_in_(3))
                    add_flag(o_ptr->art_flags, TR_SLAY_HUMAN);
            }
            break;
        case EGO_WEAPON_HELL_LANCE:
            if ( !object_is_(o_ptr, TV_POLEARM, SV_LANCE)
              && !object_is_(o_ptr, TV_POLEARM, SV_HEAVY_LANCE) )
            {
                done = FALSE;
            }
            else
            {
                while (one_in_(o_ptr->dd * 4)) { o_ptr->dd++; }
                one_demon_resistance(o_ptr);
                if (one_in_(16))
                    add_flag(o_ptr->art_flags, TR_VAMPIRIC);
                if (one_in_(ACTIVATION_CHANCE))
                    effect_add_random(o_ptr, BIAS_DEMON);
            }
            break;
        case EGO_WEAPON_HOLY_LANCE:
            if ( !object_is_(o_ptr, TV_POLEARM, SV_LANCE)
              && !object_is_(o_ptr, TV_POLEARM, SV_HEAVY_LANCE) )
            {
                done = FALSE;
            }
            else
            {
                while (one_in_(o_ptr->dd * 5)) { o_ptr->dd++; }
                one_holy_resistance(o_ptr);
                if (one_in_(77))
                {
                    o_ptr->dd = o_ptr->dd * o_ptr->ds;
                    o_ptr->ds = 1;
                    add_flag(o_ptr->art_flags, TR_ORDER);

                }
                if (one_in_(ACTIVATION_CHANCE))
                    effect_add_random(o_ptr, BIAS_PRIESTLY);
            }
            break;
        }
    }
    /* Hack -- Super-charge the damage dice, but only if they haven't already
       been boosted/altered by the ego type (e.g., Armageddon, Hell Lance, Wild, Order, etc) */
    if ( o_ptr->dd == k_info[o_ptr->k_idx].dd
      && o_ptr->ds == k_info[o_ptr->k_idx].ds
      && o_ptr->name2 != EGO_WEAPON_EXTRA_ATTACKS
      && o_ptr->name2 != EGO_WEAPON_WILD
      && o_ptr->name2 != EGO_WEAPON_ORDER )
    {
        if (o_ptr->dd * o_ptr->ds > 0 && one_in_(5 + 200/MAX(level, 1)))
        {
            do
            {
                o_ptr->dd++;
            }
            while (one_in_(o_ptr->dd * o_ptr->ds / 2));
        }
    }
    ego_weapon_adjust_weight(o_ptr);
}
void obj_create_weapon(object_type *o_ptr, int level, int power, int mode)
{
    int tohit1 = randint1(5) + m_bonus(5, level);
    int todam1 = randint1(5) + m_bonus(5, level);

    int tohit2 = m_bonus(10, level);
    int todam2 = m_bonus(10, level);
    bool crafting = (mode & AM_CRAFTING) ? TRUE : FALSE;

    if (object_is_ammo(o_ptr))
    {
        tohit2 = (tohit2+1)/2;
        todam2 = (todam2+1)/2;
    }

    if (object_is_(o_ptr, TV_SWORD, SV_DIAMOND_EDGE))
    {
        if (!crafting && power >= 2 && !one_in_(7)) return;
    }
    if (object_is_(o_ptr, TV_SWORD, SV_POISON_NEEDLE)) return;

    if (!crafting && !object_is_(o_ptr, TV_BOW, SV_HARP))
    {
        if (power == -1)
        {
            o_ptr->to_h -= tohit1;
            o_ptr->to_d -= todam1;
            if (power < -1)
            {
                o_ptr->to_h -= tohit2;
                o_ptr->to_d -= todam2;
            }

            if (o_ptr->to_h + o_ptr->to_d < 0) o_ptr->curse_flags |= TRC_CURSED;
        }
        else if (power)
        {
            o_ptr->to_h += tohit1;
            o_ptr->to_d += todam1;
            if (power > 1 || power < -1)
            {
                o_ptr->to_h += tohit2;
                o_ptr->to_d += todam2;
            }
        }
    }

    if (-1 <= power && power <= 1)
        return;

    if (mode & AM_FORCE_EGO)
        crafting = TRUE; /* Hack to prevent artifacts */

    switch (o_ptr->tval)
    {
    case TV_BOW:
        if ((!crafting && one_in_(20)) || power > 2)
            _art_create_random(o_ptr, level, power);
        else if (o_ptr->sval == SV_HARP)
            _ego_create_harp(o_ptr, level);
        else
            _ego_create_bow(o_ptr, level);
        break;

    case TV_BOLT:
    case TV_ARROW:
    case TV_SHOT:
        if (power < 0)
            break;

        o_ptr->name2 = ego_choose_type(EGO_TYPE_AMMO, level);

        switch (o_ptr->name2)
        {
        case EGO_AMMO_SLAYING:
            o_ptr->dd++;
            break;
        }

        /* Hack -- super-charge the damage dice */
        while (one_in_(10 * o_ptr->dd * o_ptr->ds))
            o_ptr->dd++;

        if (o_ptr->dd > 9)
            o_ptr->dd = 9;
        break;

    case TV_DIGGING:
        if ((!crafting && one_in_(30)) || power > 2)
            _art_create_random(o_ptr, level, power);
        else
            _ego_create_digger(o_ptr, level);
        break;

    case TV_HAFTED:
    case TV_POLEARM:
    case TV_SWORD:
        if ((!crafting && one_in_(40)) || power > 2)
            _art_create_random(o_ptr, level, power);
        else
            _ego_create_weapon(o_ptr, level);
        break;
    }
    ego_finalize(o_ptr, level, power, mode);
}

/*************************************************************************
 * Armor
 *************************************************************************/
static void _ego_create_dragon_armor(object_type *o_ptr, int level)
{
    bool done = FALSE;
    while (!done)
    {
        o_ptr->name2 = ego_choose_type(EGO_TYPE_DRAGON_ARMOR, level);
        done = TRUE;

        switch (o_ptr->name2)
        {
        case EGO_DRAGON_LORE:
            if (one_in_(3))
            {
                if (one_in_(2)) add_esp_strong(o_ptr);
                else add_esp_weak(o_ptr, FALSE);
            }
            if (one_in_(7))
                add_flag(o_ptr->art_flags, TR_MAGIC_MASTERY);
            if (one_in_(ACTIVATION_CHANCE))
            {   /* Only do strong effects since we loose the DSM's breathe activation! */
                int choices[] = {
                    EFFECT_IDENTIFY_FULL, EFFECT_DETECT_ALL, EFFECT_ENLIGHTENMENT,
                    EFFECT_CLAIRVOYANCE, EFFECT_SELF_KNOWLEDGE, -1
                };
                _effect_add_list(o_ptr, choices);
            }
            break;

        case EGO_DRAGON_BREATH:
            o_ptr->activation = k_info[o_ptr->k_idx].activation;
            o_ptr->activation.cost /= 2;  /* Timeout */
            o_ptr->activation.extra *= 2; /* Damage */
            break;

        case EGO_DRAGON_ATTACK:
            o_ptr->to_h += 3;
            o_ptr->to_d += 3;
            if (one_in_(3))
                add_flag(o_ptr->art_flags, TR_RES_FEAR);
            break;

        case EGO_DRAGON_ARMOR:
            o_ptr->to_a += 5;
            if (one_in_(3))
                o_ptr->to_a += m_bonus(10, level);
            while(one_in_(2))
                one_sustain(o_ptr);
            if (one_in_(7))
                add_flag(o_ptr->art_flags, TR_REFLECT);
            if (one_in_(7))
                add_flag(o_ptr->art_flags, TR_SH_SHARDS);
            break;

        case EGO_DRAGON_DOMINATION:
            break;

        case EGO_DRAGON_CRUSADE:
            if (o_ptr->sval != SV_DRAGON_GOLD && o_ptr->sval != SV_DRAGON_LAW)
                done = FALSE;
            else
            {
                if (one_in_(7))
                    add_flag(o_ptr->art_flags, TR_BLOWS);
            }
            break;

        case EGO_DRAGON_DEATH:
            if (o_ptr->sval != SV_DRAGON_CHAOS)
                done = FALSE;
            else
            {
                if (one_in_(6))
                    add_flag(o_ptr->art_flags, TR_VAMPIRIC);
                if (one_in_(ACTIVATION_CHANCE))
                {   /* Only do strong effects since we loose the DSM's breathe activation! */
                    int choices[] = {
                        EFFECT_GENOCIDE, EFFECT_MASS_GENOCIDE, EFFECT_WRAITHFORM,
                        EFFECT_DARKNESS_STORM, -1
                    };
                    _effect_add_list(o_ptr, choices);
                }
            }
            break;
        }
    }
}
static void _ego_create_gloves(object_type *o_ptr, int level)
{
    o_ptr->name2 = ego_choose_type(EGO_TYPE_GLOVES, level);
    switch (o_ptr->name2)
    {
    case EGO_GLOVES_GIANT:
        if (one_in_(4))
        {
            switch (randint1(3))
            {
            case 1: add_flag(o_ptr->art_flags, TR_RES_SOUND); break;
            case 2: add_flag(o_ptr->art_flags, TR_RES_SHARDS); break;
            case 3: add_flag(o_ptr->art_flags, TR_RES_CHAOS); break;
            }
        }
        if (one_in_(3))
            add_flag(o_ptr->art_flags, TR_VULN_CONF);
        if (one_in_(2))
            add_flag(o_ptr->art_flags, TR_DEC_STEALTH);
        if (one_in_(2))
            add_flag(o_ptr->art_flags, TR_DEC_DEX);
        break;
    case EGO_GLOVES_WIZARD:
        if (one_in_(4))
        {
            switch (randint1(3))
            {
            case 1: add_flag(o_ptr->art_flags, TR_RES_CONF); break;
            case 2: add_flag(o_ptr->art_flags, TR_RES_BLIND); break;
            case 3: add_flag(o_ptr->art_flags, TR_RES_LITE); break;
            }
        }
        if (one_in_(2))
            add_flag(o_ptr->art_flags, TR_DEC_STR);
        if (one_in_(3))
            add_flag(o_ptr->art_flags, TR_DEC_CON);
        if (one_in_(30))
            add_flag(o_ptr->art_flags, TR_DEVICE_POWER);
        break;
    case EGO_GLOVES_YEEK:
        if (one_in_(10))
            add_flag(o_ptr->art_flags, TR_IM_ACID);
        break;
    case EGO_GLOVES_THIEF:
        if (one_in_(20))
            add_flag(o_ptr->art_flags, TR_SPEED);
        break;
    case EGO_GLOVES_BERSERKER:
        o_ptr->to_h = -10;
        o_ptr->to_d = 10;
        o_ptr->to_a = -10;
        break;
    case EGO_GLOVES_SNIPER:
        o_ptr->to_h = 5 + randint1(10);
        break;
    }
}
static void _ego_create_robe(object_type *o_ptr, int level)
{
    o_ptr->name2 = ego_choose_type(EGO_TYPE_ROBE, level);
    switch (o_ptr->name2)
    {
    case EGO_ROBE_TWILIGHT:
        o_ptr->k_idx = lookup_kind(TV_SOFT_ARMOR, SV_YOIYAMI_ROBE);
        o_ptr->sval = SV_YOIYAMI_ROBE;
        o_ptr->ac = 0;
        o_ptr->to_a = 0;
        break;
    case EGO_ROBE_SORCERER:
        one_high_resistance(o_ptr);
        one_high_resistance(o_ptr);
        one_high_resistance(o_ptr);
        break;
    }
}
static void _ego_create_armor_protection(object_type *o_ptr, int level)
{
    assert(o_ptr->name2 == EGO_ARMOR_PROTECTION);
    if (one_in_(3))
        o_ptr->to_a += m_bonus(10, level);
}
static void _ego_create_armor_elemental_protection(object_type *o_ptr, int level)
{
    int rolls, i;
    assert(o_ptr->name2 == EGO_ARMOR_ELEMENTAL_PROTECTION);

    if (object_is_body_armour(o_ptr))
        rolls = 1 + m_bonus(6, level);
    else
        rolls = 1 + m_bonus(5, level);

    for (i = 0; i < rolls; i++)
        one_ele_resistance(o_ptr);
    if (level > 20 && one_in_(4))
        add_flag(o_ptr->art_flags, TR_RES_POIS);

    if (one_in_(ACTIVATION_CHANCE))
        effect_add_random(o_ptr, BIAS_ELEMENTAL);
}
static void _ego_create_armor_celestial_protection(object_type *o_ptr, int level)
{
    int rolls, i;
    assert(o_ptr->name2 == EGO_ARMOR_CELESTIAL_PROTECTION);
    if (object_is_body_armour(o_ptr))
        rolls = 2 + m_bonus(5, level);
    else
        rolls = 1 + m_bonus(5, level);

    for (i = 0; i < rolls; i++)
        one_high_resistance(o_ptr);

    if (one_in_(7))
        add_flag(o_ptr->art_flags, TR_HOLD_LIFE);

    if (object_is_body_armour(o_ptr))
    {
        while (one_in_(3))
            o_ptr->to_a += randint1(3);
        if (one_in_(3))
            add_flag(o_ptr->art_flags, TR_FREE_ACT);
        if (one_in_(3))
            add_flag(o_ptr->art_flags, TR_SLOW_DIGEST);
        if (one_in_(17))
            add_flag(o_ptr->art_flags, TR_REFLECT);
    }
    else if (object_is_shield(o_ptr))
    {
        if (one_in_(5))
            add_flag(o_ptr->art_flags, TR_REFLECT);
    }
}
static void _ego_create_armor_elvenkind(object_type *o_ptr, int level)
{
    assert(o_ptr->name2 == EGO_ARMOR_ELVENKIND);
    if (one_in_(4))
    {
        add_flag(o_ptr->art_flags, TR_DEX);
        if (one_in_(3))
            add_flag(o_ptr->art_flags, TR_DEC_STR);
    }
    if (object_is_body_armour(o_ptr) && level > 60 && one_in_(7))
        add_flag(o_ptr->art_flags, TR_SPEED);
}
static void _ego_create_body_armor(object_type *o_ptr, int level)
{
    bool done = FALSE;
    while (!done)
    {
        o_ptr->name2 = ego_choose_type(EGO_TYPE_BODY_ARMOR, level);
        done = TRUE;

        switch (o_ptr->name2)
        {
        case EGO_ARMOR_PROTECTION:
            _ego_create_armor_protection(o_ptr, level);
            break;
        case EGO_ARMOR_ELEMENTAL_PROTECTION:
            _ego_create_armor_elemental_protection(o_ptr, level);
            break;
        case EGO_ARMOR_CELESTIAL_PROTECTION:
            _ego_create_armor_celestial_protection(o_ptr, level);
            break;
        case EGO_ARMOR_ELVENKIND:
            _ego_create_armor_elvenkind(o_ptr, level);
            break;
        case EGO_BODY_DWARVEN:
            if (o_ptr->tval != TV_HARD_ARMOR || o_ptr->sval == SV_RUSTY_CHAIN_MAIL)
                done = FALSE;
            else
            {
                o_ptr->weight = (2 * k_info[o_ptr->k_idx].weight / 3);
                o_ptr->ac = k_info[o_ptr->k_idx].ac + 5;
                if (one_in_(4))
                    add_flag(o_ptr->art_flags, TR_CON);
                if (one_in_(4))
                    add_flag(o_ptr->art_flags, TR_DEC_DEX);
                if (one_in_(4))
                    add_flag(o_ptr->art_flags, TR_DEC_STEALTH);
                if (one_in_(2))
                    add_flag(o_ptr->art_flags, TR_REGEN);
            }
            break;
        case EGO_BODY_URUK_HAI:
            if (o_ptr->tval != TV_HARD_ARMOR)
                done = FALSE;
            else
            {
                if (one_in_(4))
                    add_flag(o_ptr->art_flags, TR_DEC_STEALTH);
            }
            break;
        case EGO_BODY_OLOG_HAI:
            if (o_ptr->tval != TV_HARD_ARMOR)
                done = FALSE;
            else
            {
                if (one_in_(4))
                    add_flag(o_ptr->art_flags, TR_CON);
                if (one_in_(4))
                    add_flag(o_ptr->art_flags, TR_DEC_STEALTH);
            }
            break;
        case EGO_BODY_DEMON:
        case EGO_BODY_DEMON_LORD:
            if (o_ptr->tval != TV_HARD_ARMOR)
                done = FALSE;
            else
            {
                if (level > 66 && one_in_(6))
                    add_flag(o_ptr->art_flags, TR_SPEED);
                if (one_in_(ACTIVATION_CHANCE))
                    effect_add_random(o_ptr, BIAS_DEMON);
            }
            break;
        }
    }
}
static void _ego_create_shield(object_type *o_ptr, int level)
{
    bool done = FALSE;
    while (!done)
    {
        o_ptr->name2 = ego_choose_type(EGO_TYPE_SHIELD, level);
        done = TRUE;

        switch (o_ptr->name2)
        {
        case EGO_ARMOR_PROTECTION:
            _ego_create_armor_protection(o_ptr, level);
            break;
        case EGO_ARMOR_ELEMENTAL_PROTECTION:
            _ego_create_armor_elemental_protection(o_ptr, level);
            break;
        case EGO_ARMOR_CELESTIAL_PROTECTION:
            _ego_create_armor_celestial_protection(o_ptr, level);
            break;
        case EGO_ARMOR_ELVENKIND:
            _ego_create_armor_elvenkind(o_ptr, level);
            break;
        case EGO_SHIELD_REFLECTION:
            if (o_ptr->sval == SV_MIRROR_SHIELD)
                done = FALSE;
            break;
        case EGO_SHIELD_ORCISH:
            if ( o_ptr->sval == SV_DRAGON_SHIELD
              || o_ptr->sval == SV_MIRROR_SHIELD )
            {
                done = FALSE;
            }
            break;
        case EGO_SHIELD_DWARVEN:
            if ( o_ptr->sval == SV_SMALL_LEATHER_SHIELD
              || o_ptr->sval == SV_LARGE_LEATHER_SHIELD
              || o_ptr->sval == SV_DRAGON_SHIELD
              || o_ptr->sval == SV_MIRROR_SHIELD )
            {
                done = FALSE;
            }
            else
            {
                o_ptr->weight = (2 * k_info[o_ptr->k_idx].weight / 3);
                o_ptr->ac = k_info[o_ptr->k_idx].ac + 4;
                if (one_in_(4))
                    add_flag(o_ptr->art_flags, TR_SUST_CON);
            }
            break;
        case EGO_SHIELD_ENDURANCE:
            if (one_in_(3))
                add_flag(o_ptr->art_flags, TR_SUST_CON);
            break;
        }
    }
}
static void _ego_create_crown(object_type *o_ptr, int level)
{
    o_ptr->name2 = ego_choose_type(EGO_TYPE_CROWN, level);
    switch (o_ptr->name2)
    {
    case EGO_CROWN_TELEPATHY:
        if (add_esp_strong(o_ptr)) add_esp_weak(o_ptr, TRUE);
        else add_esp_weak(o_ptr, FALSE);
        break;
    case EGO_CROWN_MAGI:
        if (one_in_(3))
        {
            one_high_resistance(o_ptr);
        }
        else
        {
            one_ele_resistance(o_ptr);
            one_ele_resistance(o_ptr);
            one_ele_resistance(o_ptr);
            one_ele_resistance(o_ptr);
        }
        if (one_in_(7))
            add_flag(o_ptr->art_flags, TR_EASY_SPELL);
        if (one_in_(3))
            add_flag(o_ptr->art_flags, TR_DEC_STR);

        if (one_in_(5))
            add_flag(o_ptr->art_flags, TR_MAGIC_MASTERY);
        else if (one_in_(66))
        {
            add_flag(o_ptr->art_flags, TR_SPELL_POWER);
            add_flag(o_ptr->art_flags, TR_DEC_CON);
        }
        else if (one_in_(3))
        {
            o_ptr->to_d += 4 + randint1(11);
            while (one_in_(2))
                o_ptr->to_d++;

            add_flag(o_ptr->art_flags, TR_SHOW_MODS);
        }

        if (level > 70 && one_in_(10))
            add_flag(o_ptr->art_flags, TR_SPEED);

        if (one_in_(ACTIVATION_CHANCE))
            effect_add_random(o_ptr, BIAS_MAGE);
        break;
    case EGO_CROWN_LORDLINESS:
        if (one_in_(5))
            add_flag(o_ptr->art_flags, TR_SPELL_CAP);
        if (one_in_(5))
            one_high_resistance(o_ptr);
        if (one_in_(5))
            one_high_resistance(o_ptr);
        if (level > 70 && one_in_(5))
            add_flag(o_ptr->art_flags, TR_SPEED);
        if (one_in_(ACTIVATION_CHANCE))
            effect_add_random(o_ptr, BIAS_PRIESTLY);
        break;
    case EGO_CROWN_MIGHT:
        if (one_in_(5))
        {
            o_ptr->to_h += randint1(7);
            o_ptr->to_d += randint1(7);
        }
        if (level > 70 && one_in_(10))
            add_flag(o_ptr->art_flags, TR_SPEED);
        if (one_in_(ACTIVATION_CHANCE))
            effect_add_random(o_ptr, BIAS_WARRIOR);
        break;
    case EGO_ARMOR_SEEING:
        if (one_in_(3))
        {
            if (one_in_(2)) add_esp_strong(o_ptr);
            else add_esp_weak(o_ptr, FALSE);
        }
        break;
    case EGO_ARMOR_CELESTIAL_PROTECTION:
        _ego_create_armor_celestial_protection(o_ptr, level);
        break;
    }
}
static void _ego_create_helmet(object_type *o_ptr, int level)
{
    bool done = FALSE;
    while (!done)
    {
        o_ptr->name2 = ego_choose_type(EGO_TYPE_HELMET, level);
        done = TRUE;

        switch (o_ptr->name2)
        {
        case EGO_ARMOR_PROTECTION:
            _ego_create_armor_protection(o_ptr, level);
            break;
        case EGO_ARMOR_SEEING:
            if (one_in_(7))
            {
                if (one_in_(2)) add_esp_strong(o_ptr);
                else add_esp_weak(o_ptr, FALSE);
            }
            break;
        case EGO_HELMET_DWARVEN:
            if (o_ptr->sval == SV_HARD_LEATHER_CAP || o_ptr->sval == SV_DRAGON_HELM)
            {
                done = FALSE;
                break;
            }
            o_ptr->weight = (2 * k_info[o_ptr->k_idx].weight / 3);
            o_ptr->ac = k_info[o_ptr->k_idx].ac + 3;
            if (one_in_(4))
                add_flag(o_ptr->art_flags, TR_TUNNEL);
            break;

        case EGO_HELMET_SUNLIGHT:
            if (one_in_(3))
                add_flag(o_ptr->art_flags, TR_VULN_DARK);
            if (one_in_(ACTIVATION_CHANCE))
            {
                int choices[] = {
                    EFFECT_LITE_AREA, EFFECT_LITE_MAP_AREA, EFFECT_BOLT_LITE, EFFECT_BEAM_LITE_WEAK,
                    EFFECT_BEAM_LITE, EFFECT_BALL_LITE, EFFECT_BREATHE_LITE, EFFECT_CONFUSING_LITE, -1
                };
                _effect_add_list(o_ptr, choices);
            }
            break;

        case EGO_HELMET_KNOWLEDGE:
            if (one_in_(7))
                add_flag(o_ptr->art_flags, TR_MAGIC_MASTERY);
            if (one_in_(ACTIVATION_CHANCE))
            {
                int choices[] = {
                    EFFECT_IDENTIFY, EFFECT_IDENTIFY_FULL, EFFECT_PROBING, EFFECT_DETECT_TRAPS,
                    EFFECT_DETECT_MONSTERS, EFFECT_DETECT_OBJECTS, EFFECT_DETECT_ALL,
                    EFFECT_ENLIGHTENMENT, EFFECT_CLAIRVOYANCE, EFFECT_SELF_KNOWLEDGE, -1
                };
                _effect_add_list(o_ptr, choices);
            }
            break;
        case EGO_HELMET_PIETY:
            if (one_in_(ACTIVATION_CHANCE))
            {
                int choices[] = {
                    EFFECT_HEAL, EFFECT_CURING, EFFECT_RESTORE_STATS, EFFECT_RESTORE_EXP,
                    EFFECT_HEAL_CURING, EFFECT_CURE_POIS, EFFECT_CURE_FEAR,
                    EFFECT_REMOVE_CURSE, EFFECT_REMOVE_ALL_CURSE, EFFECT_CLARITY, -1
                };
                _effect_add_list(o_ptr, choices);
            }
            break;
        case EGO_HELMET_RAGE:
            o_ptr->to_d += 3;
            o_ptr->to_d += m_bonus(7, level);
            break;
        case EGO_HELMET_VAMPIRE:
            if (one_in_(6))
                add_flag(o_ptr->art_flags, TR_VAMPIRIC);
            break;
        }
    }
}
static void _ego_create_cloak(object_type *o_ptr, int level)
{
    o_ptr->name2 = ego_choose_type(EGO_TYPE_CLOAK, level);
    switch (o_ptr->name2)
    {
    case EGO_ARMOR_PROTECTION:
        _ego_create_armor_protection(o_ptr, level);
        break;
    case EGO_ARMOR_ELEMENTAL_PROTECTION:
        _ego_create_armor_elemental_protection(o_ptr, level);
        break;
    case EGO_CLOAK_IMMOLATION:
        if (one_in_(ACTIVATION_CHANCE))
            effect_add_random(o_ptr, BIAS_FIRE);
        break;
    case EGO_CLOAK_ELECTRICITY:
        if (one_in_(ACTIVATION_CHANCE))
            effect_add_random(o_ptr, BIAS_ELEC);
        break;
    case EGO_CLOAK_FREEZING:
        if (one_in_(ACTIVATION_CHANCE))
            effect_add_random(o_ptr, BIAS_COLD);
        break;
    case EGO_CLOAK_BAT:
        o_ptr->to_d -= 6;
        o_ptr->to_h -= 6;
        if (one_in_(3))
        {
            add_flag(o_ptr->art_flags, TR_VULN_LITE);
            one_high_resistance(o_ptr);
            if (one_in_(3))
                add_flag(o_ptr->art_flags, TR_DEC_STR);
        }
        break;
    case EGO_CLOAK_NAZGUL:
        o_ptr->to_d += 6;
        o_ptr->to_h += 6;
        if (one_in_(6))
            o_ptr->curse_flags |= TRC_PERMA_CURSE;
        if (one_in_(66))
            add_flag(o_ptr->art_flags, TR_IM_COLD);
        while (one_in_(6))
            one_high_resistance(o_ptr);
        if (one_in_(ACTIVATION_CHANCE))
            effect_add_random(o_ptr, BIAS_NECROMANTIC);
        break;
    case EGO_CLOAK_RETRIBUTION:
        if (one_in_(2))
            add_flag(o_ptr->art_flags, TR_SH_FIRE);
        if (one_in_(2))
            add_flag(o_ptr->art_flags, TR_SH_COLD);
        if (one_in_(2))
            add_flag(o_ptr->art_flags, TR_SH_ELEC);
        if (one_in_(7))
            add_flag(o_ptr->art_flags, TR_SH_SHARDS);
        break;
    }
}
static void _ego_create_boots(object_type *o_ptr, int level)
{
    bool done = FALSE;
    while (!done)
    {
        o_ptr->name2 = ego_choose_type(EGO_TYPE_BOOTS, level);

        done = TRUE;

        switch (o_ptr->name2)
        {
        case EGO_BOOTS_SPEED:
        {
            int amt = 3;
            if (level >= 30)
                amt += (MIN(90,level) - 30)/10;
            o_ptr->pval = 1 + m_bonus(amt, level);
            break;
        }
        case EGO_BOOTS_FEANOR:
            o_ptr->pval = 6 + m_bonus(9, level);
            break;
        case EGO_BOOTS_DWARVEN:
            if (o_ptr->sval != SV_PAIR_OF_METAL_SHOD_BOOTS)
            {
                done = FALSE;
                break;
            }
            o_ptr->weight = (2 * k_info[o_ptr->k_idx].weight / 3);
            o_ptr->ac = k_info[o_ptr->k_idx].ac + 4;
            if (one_in_(4))
                add_flag(o_ptr->art_flags, TR_SUST_CON);
            break;
        case EGO_BOOTS_ELVENKIND:
            if (one_in_(2))
                one_high_resistance(o_ptr);
            if (one_in_(2))
                add_flag(o_ptr->art_flags, TR_LEVITATION);
            break;
        case EGO_BOOTS_LEVITATION:
        case EGO_BOOTS_SPRITE:
            if (one_in_(2))
                one_high_resistance(o_ptr);
            break;
        }
    }
}

void obj_create_armor(object_type *o_ptr, int level, int power, int mode)
{
    int toac1 = randint1(5) + m_bonus(5, level);
    int toac2 = m_bonus(10, level);
    bool crafting = (mode & AM_CRAFTING) ? TRUE : FALSE;

    if (!crafting)
    {
        if (power == -1)
        {
            o_ptr->to_a -= toac1;
            if (power < -1)
                o_ptr->to_a -= toac2;

            if (o_ptr->to_a < 0) o_ptr->curse_flags |= TRC_CURSED;
        }
        else if (power)
        {
            o_ptr->to_a += toac1;
            if (power > 1 || power < -1)
                o_ptr->to_a += toac2;
        }
    }

    if (-1 <= power && power <= 1)
        return;

    if (mode & AM_FORCE_EGO)
        crafting = TRUE; /* Hack to prevent artifacts */

    switch (o_ptr->tval)
    {
    case TV_DRAG_ARMOR:
        if ((!crafting && one_in_(50)) || power > 2)
            _art_create_random(o_ptr, level, power);
        else
            _ego_create_dragon_armor(o_ptr, level);
        break;

    case TV_GLOVES:
        if ((!crafting && one_in_(20)) || power > 2)
            _art_create_random(o_ptr, level, power);
        else
            _ego_create_gloves(o_ptr, level);
        break;

    case TV_HARD_ARMOR:
    case TV_SOFT_ARMOR:
        if (object_is_(o_ptr, TV_SOFT_ARMOR, SV_ROBE) && one_in_(7))
            _ego_create_robe(o_ptr, level);
        else if ((!crafting && one_in_(20)) || power > 2)
            _art_create_random(o_ptr, level, power);
        else
            _ego_create_body_armor(o_ptr, level);
        break;

    case TV_SHIELD:
        if ((!crafting && one_in_(20)) || power > 2)
            _art_create_random(o_ptr, level, power);
        else
            _ego_create_shield(o_ptr, level);
        break;

    case TV_CROWN:
        if ((!crafting && one_in_(20)) || power > 2)
            _art_create_random(o_ptr, level, power);
        else
            _ego_create_crown(o_ptr, level);
        break;
    case TV_HELM:
        if ((!crafting && one_in_(20)) || power > 2)
            _art_create_random(o_ptr, level, power);
        else
            _ego_create_helmet(o_ptr, level);
        break;

    case TV_CLOAK:
        if ((!crafting && one_in_(20)) || power > 2)
            _art_create_random(o_ptr, level, power);
        else
            _ego_create_cloak(o_ptr, level);
        break;

    case TV_BOOTS:
        if ((!crafting && one_in_(20)) || power > 2)
            _art_create_random(o_ptr, level, power);
        else
            _ego_create_boots(o_ptr, level);
        break;
    }
    ego_finalize(o_ptr, level, power, mode);
}

/*************************************************************************
 * Lites
 *************************************************************************/
void obj_create_lite(object_type *o_ptr, int level, int power, int mode)
{
    bool done = FALSE;

    /* Hack -- Torches and Lanterns -- random fuel */
    if (o_ptr->sval == SV_LITE_TORCH || o_ptr->sval == SV_LITE_LANTERN)
    {
        if (o_ptr->pval > 0) o_ptr->xtra4 = randint1(o_ptr->pval);
        o_ptr->pval = 0;

        if (power == 1 && one_in_(3))
            power++;
    }

    if (-1 <= power && power <= 1)
        return;

    if (o_ptr->sval == SV_LITE_FEANOR && (one_in_(7) || power > 2))
    {
        _art_create_random(o_ptr, level, power);
        return;
    }

    while (!done)
    {
        o_ptr->name2 = ego_choose_type(EGO_TYPE_LITE, level);
        done = TRUE;
        switch (o_ptr->name2)
        {
        case EGO_LITE_DURATION:
            if (o_ptr->sval == SV_LITE_FEANOR)
                done = FALSE;
            break;
        case EGO_LITE_VALINOR:
            if (o_ptr->sval != SV_LITE_FEANOR)
                done = FALSE;
            else if (one_in_(7))
                add_flag(o_ptr->art_flags, TR_STEALTH);
            break;
        case EGO_LITE_DARKNESS:
            o_ptr->xtra4 = 0;
            break;
        }
    }
    ego_finalize(o_ptr, level, power, mode);
}

void ego_finalize(object_type *o_ptr, int level, int power, int mode)
{
    if (object_is_ego(o_ptr))
    {
        ego_type *e_ptr = &e_info[o_ptr->name2];

        if (!store_hack)
            e_ptr->counts.generated++;

        /* Hack -- acquire "cursed" flag */
        if (e_ptr->gen_flags & TRG_CURSED) o_ptr->curse_flags |= (TRC_CURSED);
        if (e_ptr->gen_flags & TRG_HEAVY_CURSE) o_ptr->curse_flags |= (TRC_HEAVY_CURSE);
        if (e_ptr->gen_flags & TRG_PERMA_CURSE) o_ptr->curse_flags |= (TRC_PERMA_CURSE);
        if (e_ptr->gen_flags & (TRG_RANDOM_CURSE0)) o_ptr->curse_flags |= get_curse(0, o_ptr);
        if (e_ptr->gen_flags & (TRG_RANDOM_CURSE1)) o_ptr->curse_flags |= get_curse(1, o_ptr);
        if (e_ptr->gen_flags & (TRG_RANDOM_CURSE2)) o_ptr->curse_flags |= get_curse(2, o_ptr);

        if (e_ptr->gen_flags & (TRG_ONE_SUSTAIN)) one_sustain(o_ptr);
        if (e_ptr->gen_flags & (TRG_XTRA_POWER)) one_ability(o_ptr);
        if (e_ptr->gen_flags & (TRG_XTRA_H_RES))
        {
            one_high_resistance(o_ptr);
            if (randint1(level) > 60)
                one_high_resistance(o_ptr);
        }
        if (e_ptr->gen_flags & (TRG_XTRA_E_RES)) one_ele_resistance(o_ptr);
        if (e_ptr->gen_flags & (TRG_XTRA_D_RES)) one_dragon_ele_resistance(o_ptr);
        if (e_ptr->gen_flags & (TRG_XTRA_L_RES)) one_lordly_high_resistance(o_ptr);
        if (e_ptr->gen_flags & (TRG_XTRA_RES)) one_resistance(o_ptr);

        /* Hack -- obtain bonuses */
        if (e_ptr->max_to_h)
        {
            if (e_ptr->max_to_h < 0)
                o_ptr->to_h -= randint1(-e_ptr->max_to_h);
            else
                o_ptr->to_h += randint1(e_ptr->max_to_h);
        }
        if (e_ptr->max_to_d)
        {
            if (e_ptr->max_to_d < 0)
                o_ptr->to_d -= randint1(-e_ptr->max_to_d);
            else
                o_ptr->to_d += randint1(e_ptr->max_to_d);
        }
        if (e_ptr->max_to_a)
        {
            if (e_ptr->max_to_a < 0)
                o_ptr->to_a -= randint1(-e_ptr->max_to_a);
            else
                o_ptr->to_a += randint1(e_ptr->max_to_a);
        }

        /* Hack -- obtain pval */
        if (e_ptr->max_pval)
        {
            if ((o_ptr->name2 == EGO_WEAPON_CRUSADE) && (have_flag(o_ptr->art_flags, TR_BLOWS)))
            {
                if (o_ptr->dd*o_ptr->ds > 30)
                {
                    remove_flag(o_ptr->art_flags, TR_BLOWS);
                    /* As far as I can tell, pval is first assigned for Holy Avengers
                        as the terminal else clause of this (outer) if, which is missed
                        by having picked up extra blows ... */
                    o_ptr->pval = randint1(e_ptr->max_pval);
                }
                else
                {
                    o_ptr->pval = randint1(2);
                    if ((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_HAYABUSA))
                        o_ptr->pval += randint1(2);
                    if ((level > 60) && one_in_(3) && ((o_ptr->dd*(o_ptr->ds+1)) < 15)) o_ptr->pval += randint1(2);
                }
            }
            else if (o_ptr->name2 == EGO_WEAPON_EXTRA_ATTACKS)
            {
                o_ptr->pval = randint1(e_ptr->max_pval*level/100+1);
                if (o_ptr->pval > 6) o_ptr->pval = 6;
                if (o_ptr->pval == 6 && !one_in_(o_ptr->dd * o_ptr->ds / 2)) o_ptr->pval = 5;
                if ((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_HAYABUSA))
                    o_ptr->pval += randint1(2);

                if (o_ptr->dd*o_ptr->ds > 30)
                    o_ptr->pval = MAX(o_ptr->pval, 3);
            }
            else if ( o_ptr->name2 == EGO_CLOAK_BAT
                   || o_ptr->name2 == EGO_CLOAK_COWARDICE
                   || o_ptr->name2 == EGO_CLOAK_AMAN )
            {
                o_ptr->pval = randint1(e_ptr->max_pval);
                if (o_ptr->sval == SV_ELVEN_CLOAK) o_ptr->pval += randint0(2);
            }
            else if (o_ptr->name2 == EGO_GLOVES_BERSERKER)
            {
                o_ptr->pval = randint1(2);
                if (one_in_(15))
                    o_ptr->pval++;
            }
            else
            {
                o_ptr->pval += randint1(e_ptr->max_pval);
            }
        }

        /* pval boosting to aid end game quality */
        if (o_ptr->name2 == EGO_BOOTS_ELVENKIND && level > 70)
        {
            o_ptr->to_a += randint1(5);
            while (one_in_(3))
                o_ptr->pval++;
        }
        else if (o_ptr->name2 == EGO_CLOAK_AMAN && level > 80)
        {
            while (one_in_(4))
                o_ptr->pval++;
        }
        else if ( (o_ptr->name2 == EGO_CROWN_MAGI || o_ptr->name2 == EGO_CROWN_LORDLINESS || o_ptr->name2 == EGO_CROWN_MIGHT)
               && level > 80 )
        {
            if (one_in_(5))
                o_ptr->pval++;
        }

        if (have_flag(o_ptr->art_flags, TR_DEVICE_POWER) && o_ptr->pval >= 3)
        {
            o_ptr->pval = 2;
            if (one_in_(30)) o_ptr->pval++;
        }

        if ( object_is_(o_ptr, TV_SWORD, SV_FALCON_SWORD)
          && o_ptr->pval > 2
          && o_ptr->name2 != EGO_WEAPON_EXTRA_ATTACKS )
        {
            o_ptr->pval = 2;
        }

        {
            int i;
            for (i = 0; i < TR_FLAG_ARRAY_SIZE; i++)
            {
                /* Artifact flags that aren't also base ego flags are possible
                   extra flags for this ego type. We mark these now so that
                   we can figure out better object lore later on cursed egos */
                e_ptr->xtra_flags[i] |= o_ptr->art_flags[i] & (~e_ptr->flags[i]);
            }
        }
        /* Cursed Egos: Make sure to do this last to avoid nonsensical combinations of
           good and bad flags (e.g. resist fire and vulnerable to fire) */
        if (power == -2)
        {
            curse_object(o_ptr);
            if (!o_ptr->pval && have_pval_flag(o_ptr->art_flags))
                o_ptr->pval = randint1(3);
        }
    }
}

/*************************************************************************
 * Branding Spells
 *************************************************************************/
void ego_brand_weapon(object_type *o_ptr, int which)
{
    assert(o_ptr);
    assert(!o_ptr->name1 && !o_ptr->name2);
    assert(EGO_WEAPON_SLAYING <= which && which <= EGO_WEAPON_NOLDOR);
    apply_magic_ego = which;
    _ego_create_weapon(o_ptr, object_level);
    ego_finalize(o_ptr, object_level, 2, AM_CRAFTING);
}

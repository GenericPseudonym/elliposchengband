/* Purpose: create a player character */

/*
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies. Other copyrights may also apply.
 */

#include "angband.h"
#include "z-doc.h"


cptr realm_jouhou[VALID_REALM] =
{
"Life magic is very good for healing; it relies mostly on healing, protection and detection spells. Also life magic have a few attack spells as well. It said that some high level spell of life magic can disintegrate Undead monsters into ash.",
"Sorcery is a `meta` realm, including enchantment and general spells. It provides superb protection spells, spells to enhance your odds in combat and, most importantly, a vast selection of spells for gathering information. However, Sorcery has one weakness: it has no spells to deal direct damage to your enemies.",
"Nature magic makes you master of elements; it provides protection, detection, curing and attack spells. Nature also has a spell of Herbal Healing, which is the only powerful healing spell outside the realm of Life magic.",
"There are few types of magic more unpredictable and difficult to control than Chaos magic. Chaos is the very element of unmaking, and the Chaos spells are the most terrible weapons of destruction imaginable. The caster can also call on the primal forces of Chaos to induce mutations in his/her opponents and even him/herself.",
"There is no fouler nor more evil category of spells than the necromantic spells of Death Magic. These spells are relatively hard to learn, but at higher levels the spells give the caster power over living and the (un)dead, but the most powerful spells need his / her own blood as the focus, often hurting the caster in the process of casting.",
"Trump magic has, indeed, an admirable selection of teleportation spells. Since the Trump gateways can also be used to summon other creatures, Trump magic has an equally impressive selection of summoning spells. However, not all monsters appreciate being drawn to another place by Trump user.",
"Arcane magic is a general purpose realm of magic. It attempts to encompass all 'useful' spells from all realms. This is the downside of Arcane magic: while Arcane does have all the necessary 'tool' spells for a dungeon delver, it has no ultra-powerful high level spells. As a consequence, all Arcane spellbooks can be bought in town. It should also be noted that the 'specialized' realms usually offer the same spell at a lower level and cost. ",
"Craft magic can strengthen the caster or the equipments. These spells can greatly improve the caster's fighting ability. Using them against opponents directly is not possible.",
"Demon is a very evil realm, same as Death. It provides various attack spells and devilish detection spells. at higher levels, Demon magic provides ability to dominate demons, and to polymorph yourself into a demon.",
"Crusade is a magic of 'Justice'. It includes damage spells, which are greatly effective against foul and evil monsters, but have poor effects against good monsters.",
"Necromancy allows communication with and ultimately control over the deceased. All direct damage afforded by this realm requires the caster to touch his or her opponent. Any weapons or gloves will obstruct this macabre contact.",
"Armageddon is the most deadly offensive realm. You won't be lacking for firepower here. "
    "However, every spell is an offensive spell, so this realm suffers from a lack of any "
    "utility spells.",
"Music magic shows various effects as sing song. There is two type of song; the one which shows effects instantly and the other one shows effect continuously until SP runs out. But the latter type has a limit; only one song can be sing at the same time.",
"The books of Kendo describe about various combat techniques. When learning new techniques, you are required to carry the books, but once you memorizes them, you don't have to carry them. When using a technique, wielding a weapon is required.",
"Hex is a very terrible realm. Spells gives continual effects when they are spelled continually like songs. Spells may obstract monsters' actions, may deal damages in sight, may revenge against enemies.",
"The books of Rage describe various techniques. To learn a new technique, you must perform a ritual of rage, destroying the book in the process."
    " Once learned, you may use techniques without requiring the corresponding Rage book, but you will need to find many copies of each book in order"
    " to learn all of the techniques.",
"Burglary is the preferred realm of rogues, allowing them to specialize in what they do best: Stealing! "
    "This realm offers good detection and escapes, offers talents for picking pockets and setting traps, "
    "and even allows for direct assassination of sleeping monsters. The books for this realm are only "
    "available in the Black Market (or in the dungeon).",
};



#define AUTOROLLER_STEP 50
#define AUTOROLLER_MAX 50 * 1000
#define AUTOROLLER_DELAY
#define MAX_TRIES 100

void get_max_stats(void)
{
    int        i, j, roll;
    int        dice[6];

    /* Roll and verify some stats */
    while (TRUE)
    {
        /* Roll some dice */
        for (j = i = 0; i < 6; i++)
        {
            /* Roll the dice */
            roll = randint1(7);

            dice[i] = roll;

            /* Collect the maximum */
            j += dice[i];
        }

        /* Verify totals */
        if (j == 24) break;
    }

    /* Acquire the stats */
    for (i = 0; i < 6; i++)
    {
        j = 18 + 60 + dice[i]*10;

        /* Save that value */
        p_ptr->stat_max_max[i] = j;
        if (p_ptr->stat_max[i] > j)
            p_ptr->stat_max[i] = j;
        if (p_ptr->stat_cur[i] > j)
            p_ptr->stat_cur[i] = j;
    }
    p_ptr->knowledge &= ~(KNOW_STAT);

    /* Redisplay the stats later */
    p_ptr->redraw |= (PR_STATS);
}

int _race_exp_factor(void)
{
    if (p_ptr->prace == RACE_DOPPELGANGER)
        return get_race()->exp;
    return get_true_race()->exp;
}
int calc_exp_factor(void)
{
    int exp;
    int r_exp = _race_exp_factor();
    int c_exp = get_class()->exp;
    int a_exp = get_personality()->exp;

    if (p_ptr->prace == RACE_ANDROID)
        return r_exp;

    exp = r_exp * c_exp / 100;
    exp = exp * a_exp / 100;

    if (p_ptr->prace == RACE_MON_DRAGON)
    {
        dragon_realm_ptr realm = dragon_get_realm(p_ptr->dragon_realm);
        exp = exp * realm->exp / 100;
    }

    return exp;
}

static void e_info_reset(void)
{
}

static void k_info_reset(void)
{
    int i;

    /* Reset the "objects" */
    for (i = 1; i < max_k_idx; i++)
    {
        object_kind *k_ptr = &k_info[i];

        k_ptr->tried = FALSE;
        k_ptr->aware = FALSE;
    }
}

/*
 * Clear all the global "character" data
 */
static void player_wipe(void)
{
    int i;

    /* Hack -- free the "last message" string */
    if (p_ptr->last_message) z_string_free(p_ptr->last_message);

    /* Hack -- zero the struct */
    (void)WIPE(p_ptr, player_type);
    p_ptr->mimic_form = MIMIC_NONE;

    /* Wipe the quests */
    for (i = 0; i < max_quests; i++)
    {
        quest[i].status = QUEST_STATUS_UNTAKEN;

        quest[i].cur_num = 0;
        quest[i].max_num = 0;
        quest[i].type = 0;
        quest[i].level = 0;
        quest[i].r_idx = 0;
        quest[i].complev = 0;
        quest[i].seed = 0;
    }

    /* No weight */
    p_ptr->total_weight = 0;

    /* No items */
    inven_cnt = 0;

    /* Clear the inventory */
    for (i = 0; i < INVEN_TOTAL; i++)
    {
        object_wipe(&inventory[i]);
    }


    /* Start with no artifacts made yet */
    for (i = 0; i < max_a_idx; i++)
    {
        artifact_type *a_ptr = &a_info[i];
        a_ptr->generated = FALSE;
        a_ptr->found = FALSE;
    }

    /* Reset the objects */
    k_info_reset();
    e_info_reset();
    stats_reset();

    /* Reset the "monsters" */
    for (i = 1; i < max_r_idx; i++)
    {
        monster_race *r_ptr = &r_info[i];

        /* Hack -- Reset the counter */
        r_ptr->cur_num = 0;

        /* Hack -- Reset the max counter */
        r_ptr->max_num = 100;

        /* Hack -- Reset the max counter */
        if (r_ptr->flags1 & RF1_UNIQUE) r_ptr->max_num = 1;

        /* Hack -- Non-unique Nazguls are semi-unique */
        else if (r_ptr->flags7 & RF7_NAZGUL) r_ptr->max_num = MAX_NAZGUL_NUM;
        else if (i == MON_CAMELOT_KNIGHT) r_ptr->max_num = MAX_CAMELOT_KNIGHT_NUM;

        /* Clear visible kills in this life */
        r_ptr->r_pkills = 0;

        /* Clear all kills in this life */
        r_ptr->r_akills = 0;
        r_ptr->r_skills = 0;
        r_ptr->stolen_ct = 0;

        /* Wipe out pact alliances from previous character
        Currently, flagsr is only set to make the memory field
        work, but perhaps it would be better to set this once
        and for all when a pact is made?  This would break
        my savefiles though ...*/
        r_ptr->flagsr &= ~(RFR_PACT_MONSTER);
        r_ptr->r_flagsr &= ~(RFR_PACT_MONSTER);
    }


    /* Hack -- Well fed player */
    p_ptr->food = PY_FOOD_FULL - 1;


    /* Wipe the spells */
    if (p_ptr->pclass == CLASS_SORCERER)
    {
        p_ptr->spell_learned1 = p_ptr->spell_learned2 = 0xffffffffL;
        p_ptr->spell_worked1 = p_ptr->spell_worked2 = 0xffffffffL;
    }
    else
    {
        p_ptr->spell_learned1 = p_ptr->spell_learned2 = 0L;
        p_ptr->spell_worked1 = p_ptr->spell_worked2 = 0L;
    }
    p_ptr->spell_forgotten1 = p_ptr->spell_forgotten2 = 0L;
    for (i = 0; i < 64; i++) p_ptr->spell_order[i] = 99;
    p_ptr->learned_spells = 0;
    p_ptr->add_spells = 0;
    p_ptr->knowledge = 0;

    /* Clean the mutation count */
    mutant_regenerate_mod = 100;

    /* Clear "cheat" options */
    cheat_peek = FALSE;
    cheat_hear = FALSE;
    cheat_room = FALSE;
    cheat_xtra = FALSE;
    cheat_live = FALSE;
    cheat_save = FALSE;

    /* Assume no winning game */
    p_ptr->total_winner = FALSE;

    world_player = FALSE;

    /* Assume no panic save */
    p_ptr->panic_save = 0;

    /* Assume no cheating */
    p_ptr->noscore = 0;
    p_ptr->wizard = FALSE;

    /* Not waiting to report score */
    p_ptr->wait_report_score = FALSE;

    /* Default pet command settings */
    p_ptr->pet_follow_distance = PET_FOLLOW_DIST;
    p_ptr->pet_extra_flags = (PF_TELEPORT | PF_ATTACK_SPELL | PF_SUMMON_SPELL);

    /* Wipe the recall depths */
    for (i = 0; i < max_d_idx; i++)
    {
        max_dlv[i] = 0;
        dungeon_flags[i] = 0;
    }

    p_ptr->visit = 1;
    p_ptr->wild_mode = FALSE;

    for (i = 0; i < MAX_MAGIC_NUM; i++)
    {
        p_ptr->magic_num1[i] = 0;
        p_ptr->magic_num2[i] = 0;
    }

    /* Level one */
    p_ptr->max_plv = p_ptr->lev = 1;

    /* Initialize arena and rewards information -KMW- */
    p_ptr->arena_number = 0;
    p_ptr->inside_arena = FALSE;
    p_ptr->inside_quest = 0;
    for (i = 0; i < MAX_MANE; i++)
    {
        p_ptr->mane_spell[i] = -1;
        p_ptr->mane_dam[i] = 0;
    }
    p_ptr->mane_num = 0;
    p_ptr->exit_bldg = TRUE; /* only used for arena now -KMW- */

    /* Bounty */
    p_ptr->today_mon = 0;

    /* Reset monster arena */
    battle_monsters();

    /* Reset mutations */
    for (i = 0; i < MUT_FLAG_SIZE; ++i)
    {
        p_ptr->muta[i] = 0;
        p_ptr->muta_lock[i] = 0;
    }

    for (i = 0; i < MAX_DEMIGOD_POWERS; ++i)
        p_ptr->demigod_power[i] = -1;

    p_ptr->draconian_power = -1;

    p_ptr->duelist_target_idx = 0;

    /* Reset virtues*/
    for (i = 0; i < 8; i++) p_ptr->virtues[i]=0;

    /* Set the recall dungeon accordingly */
    if (no_wilderness)
    {
        dungeon_type = 0;
        p_ptr->recall_dungeon = DUNGEON_ANGBAND;
    }
    else
    {
        dungeon_type = 0;
        p_ptr->recall_dungeon = DUNGEON_STRONGHOLD;
    }
}


/*
 *  Hook function for quest monsters
 */
static bool mon_hook_quest(int r_idx)
{
    monster_race *r_ptr = &r_info[r_idx];

    /* Random quests are in the dungeon */
    if (r_ptr->flags8 & RF8_WILD_ONLY) return FALSE;

    /* No random quests for aquatic monsters */
    if (r_ptr->flags7 & RF7_AQUATIC) return FALSE;

    /* No random quests for multiplying monsters */
    if (r_ptr->flags2 & RF2_MULTIPLY) return FALSE;

    /* No quests to kill friendly monsters */
    if (r_ptr->flags7 & RF7_FRIENDLY) return FALSE;

    return TRUE;
}

static bool mon_hook_quest_unique(int r_idx)
{
    monster_race *r_ptr = &r_info[r_idx];

    if (!(r_ptr->flags1 & RF1_UNIQUE)) return FALSE;

    return TRUE;
}

static bool mon_hook_quest_nonunique(int r_idx)
{
    monster_race *r_ptr = &r_info[r_idx];

    if (r_ptr->flags1 & RF1_UNIQUE) return FALSE;
    if (r_ptr->flags7 & RF7_UNIQUE2) return FALSE;
    if (r_ptr->flags7 & RF7_NAZGUL) return FALSE;

    return TRUE;
}


/*
 * Determine the random quest uniques
 */
void determine_random_questor(quest_type *q_ptr)
{
    int          r_idx = 0;
    monster_race *r_ptr;
    int             attempt = 0;
    bool         force_unique = FALSE;
    bool         prevent_unique = FALSE;

    /* High Level quests are stacked with uniques. Everything else
       is stacked the other way. So lets make some attempt at balance.
       Of course, users can force all quests to be for uniques, in
       true Hengband spirit. */
    if (quest_unique || one_in_(3))
    {
        get_mon_num_prep(mon_hook_quest, mon_hook_quest_unique);
        force_unique = TRUE;
    }
    else if (one_in_(2))
    {
        get_mon_num_prep(mon_hook_quest, mon_hook_quest_nonunique);
        prevent_unique = TRUE;
    }
    else
        get_mon_num_prep(mon_hook_quest, NULL);

    while (attempt < 10000)
    {
        int min_lev = q_ptr->level + 1;
        int max_lev = q_ptr->level + 9;
        if (q_ptr->level < 10)
            max_lev -= 2;
        else if (q_ptr->level < 20)
            max_lev -= 1;
        else if (q_ptr->level > 80)
            max_lev += 2;
        else if (q_ptr->level > 70)
            max_lev += 1;
        int mon_lev = (min_lev + max_lev + 1) / 2;
        mon_lev += randint0(max_lev - mon_lev + 1);

        attempt++;

        unique_count = 0; /* Hack! */
        r_idx = get_mon_num(mon_lev);
        r_ptr = &r_info[r_idx];

        // Don't allow these uniques (mainly for being too nasty for the depth
        // at which they would be placed.
        if (r_idx == MON_ROBIN_HOOD) continue;         //6
        if (r_idx == MON_GACHAPIN) continue;           //21
        if (r_ptr->flags2 & RF2_CAMELOT) continue;     //21 or 30
        if (r_idx == MON_SHEER_HEART_ATTACK) continue; //30
        if (r_idx == MON_KAVLAX) continue;             //30
        if (r_idx == MON_JACK_SHADOWS) continue;       //39
        if (r_idx == MON_YIBB_TSTLL) continue;         //39
        if (r_idx == MON_THE_METAL_BABBLE) continue;   //76

        /* Try to enforce preferences, but its virtually impossible to prevent
           high level quests for uniques */
        if (attempt < 5000)
        {
            if (prevent_unique && (r_ptr->flags1 & RF1_UNIQUE)) continue;
            if (force_unique && !(r_ptr->flags1 & RF1_UNIQUE)) continue;
        }

        if (r_ptr->flags1 & RF1_QUESTOR) continue;

        if (r_ptr->rarity > 100) continue;

        if (r_ptr->flags7 & RF7_FRIENDLY) continue;

        if (r_ptr->flags7 & RF7_AQUATIC) continue;

        if (r_ptr->flags8 & RF8_WILD_ONLY) continue;

        if (no_questor_or_bounty_uniques(r_idx)) continue;

        if (r_ptr->level > max_lev) continue;

        if (r_ptr->level >= min_lev || attempt > 5000) break;
    }

    q_ptr->r_idx = r_idx;
    //msg_format("%d->%d: %s\n", q_ptr->level, r_info[r_idx].level, r_name + r_info[r_idx].name);
}


/*
 *  Initialize random quests and final quests
 */
static void init_dungeon_quests(void)
{
    int i;

    num_random_quests = 10; /*get_quantity("How many quests (0 to 49)? ", 49);*/

    /* Init the random quests */
    init_flags = INIT_ASSIGN;
    p_ptr->inside_quest = MIN_RANDOM_QUEST;

    process_dungeon_file("q_info.txt", 0, 0, 0, 0);

    p_ptr->inside_quest = 0;

    /* Generate quests */
    for (i = MIN_RANDOM_QUEST + num_random_quests - 1; i >= MIN_RANDOM_QUEST; i--)
    {
        quest_type      *q_ptr = &quest[i];
        monster_race    *quest_r_ptr;

        q_ptr->status = QUEST_STATUS_TAKEN;
        determine_random_questor(q_ptr);

        /* Mark uniques */
        quest_r_ptr = &r_info[q_ptr->r_idx];
        if (quest_r_ptr->flags1 & RF1_UNIQUE)
        {
            quest_r_ptr->flags1 |= RF1_QUESTOR;
            q_ptr->max_num = 1;
        }
        else
        {
            q_ptr->max_num = randint1(20) + 5;
        }
    }

    /* Init the two main quests (Oberon + Serpent) */
    init_flags = INIT_ASSIGN;
    p_ptr->inside_quest = QUEST_OBERON;

    process_dungeon_file("q_info.txt", 0, 0, 0, 0);

    quest[QUEST_OBERON].status = QUEST_STATUS_TAKEN;

    p_ptr->inside_quest = QUEST_SERPENT;

    process_dungeon_file("q_info.txt", 0, 0, 0, 0);

    quest[QUEST_SERPENT].status = QUEST_STATUS_TAKEN;
    p_ptr->inside_quest = 0;
}

/*
 * Reset turn
 */
static void init_turn(void)
{
    if ( p_ptr->prace == RACE_VAMPIRE
      || p_ptr->prace == RACE_MON_VAMPIRE
      || p_ptr->prace == RACE_SKELETON
      || p_ptr->prace == RACE_ZOMBIE
      || p_ptr->prace == RACE_SPECTRE )
    {
        /* Undead start just after midnight */
        game_turn = (TURNS_PER_TICK*3 * TOWN_DAWN) / 4 + 1;
        game_turn_limit = TURNS_PER_TICK * TOWN_DAWN * MAX_DAYS + TURNS_PER_TICK * TOWN_DAWN * 3 / 4;
    }
    else
    {
        game_turn = 1;
        game_turn_limit = TURNS_PER_TICK * TOWN_DAWN * (MAX_DAYS - 1) + TURNS_PER_TICK * TOWN_DAWN * 3 / 4;
    }

    dungeon_turn = 1;
    dungeon_turn_limit = TURNS_PER_TICK * TOWN_DAWN * (MAX_DAYS - 1) + TURNS_PER_TICK * TOWN_DAWN * 3 / 4;
}

/*
 * Hook function for human corpses
 */
bool monster_hook_human(int r_idx)
{
    monster_race *r_ptr = &r_info[r_idx];

    if (r_ptr->flags1 & (RF1_UNIQUE)) return FALSE;

    if (my_strchr("pht", r_ptr->d_char)) return TRUE;

    return FALSE;
}

cptr birth_get_realm_desc(int i)
{
    return realm_jouhou[i-1];
}

/*
 * Create a new character.
 *
 * Note that we may be called with "junk" leftover in the various
 * fields, so we must be sure to clear them first.
 */
bool birth_hack = FALSE;
void player_birth(void)
{
    int i, j;

    birth_hack = TRUE;
    playtime = 0;

    wipe_m_list();
    player_wipe();

    /* Create a new character */
    if (py_birth() != UI_OK)
        quit(NULL);

    /* Here's a bunch of crap that py_birth() shouldn't need to know */
    init_dungeon_quests();
    init_turn();

    /* Init the shops */
    for (i = 1; i < max_towns; i++)
    {
        for (j = 0; j < MAX_STORES; j++)
            store_init(i, j);
    }

    /* Generate the random seeds for the wilderness */
    seed_wilderness();

    birth_hack = FALSE;
}


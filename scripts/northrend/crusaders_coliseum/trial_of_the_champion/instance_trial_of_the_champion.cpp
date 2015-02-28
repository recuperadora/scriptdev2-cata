/* This file is part of the ScriptDev2 Project. See AUTHORS file for Copyright information
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* ScriptData
SDName: instance_trial_of_the_champion
SD%Complete: 30
SDComment: First encounter only
SDCategory: Crusader Coliseum, Trial of the Champion
EndScriptData */

#include "precompiled.h"
#include "trial_of_the_champion.h"

/* Trial of the Champion encounters:
0 - Grand Champions
1 - Argent Champion
2 - Black Knight
*/

enum
{
    // grand champions
    SAY_HERALD_HORDE_CHALLENGE              = -1650000,
    SAY_HERALD_ALLIANCE_CHALLENGE           = -1650006,

    SAY_TIRION_CHALLENGE_WELCOME            = -1650012,
    SAY_TIRION_FIRST_CHALLENGE              = -1650013,
    SAY_THRALL_ALLIANCE_CHALLENGE           = -1650014,
    SAY_GARROSH_ALLIANCE_CHALLENGE          = -1650015,
    SAY_VARIAN_HORDE_CHALLENGE              = -1650016,
    SAY_TIRION_CHALLENGE_BEGIN              = -1650017,

    // argent champion
    SAY_TIRION_ARGENT_CHAMPION              = -1650028,
    SAY_TIRION_ARGENT_CHAMPION_BEGIN        = -1650029,
    SAY_HERALD_EADRIC                       = -1650030,
    SAY_HERALD_PALETRESS                    = -1650031,
    EMOTE_HORDE_ARGENT_CHAMPION             = -1650032,
    EMOTE_ALLIANCE_ARGENT_CHAMPION          = -1650033,
    SAY_EADRIC_INTRO                        = -1650034,
    SAY_PALETRESS_INTRO_1                   = -1650035,
    SAY_PALETRESS_INTRO_2                   = -1650036,

    // black knight
    SAY_TIRION_ARGENT_CHAMPION_COMPLETE     = -1650037,
    SAY_HERALD_BLACK_KNIGHT_SPAWN           = -1650038,
    SAY_BLACK_KNIGHT_INTRO_1                = -1650039,
    SAY_TIRION_BLACK_KNIGHT_INTRO_2         = -1650040,
    SAY_BLACK_KNIGHT_INTRO_3                = -1650041,
    SAY_BLACK_KNIGHT_INTRO_4                = -1650042,

    // event complete
    SAY_TIRION_EPILOG_1                     = -1650043,
    SAY_TIRION_EPILOG_2                     = -1650044,
    SAY_VARIAN_ALLIANCE_EPILOG_3            = -1650045,
    SAY_THRALL_HORDE_EPILOG_3               = -1650046,

    // other texts
    SAY_GARROSH_OTHER_1                     = -1650047,
    SAY_THRALL_OTHER_2                      = -1650048,
    SAY_GARROSH_OTHER_3                     = -1650049,
    SAY_VARIAN_OTHER_4                      = -1650050,
    SAY_VARIAN_OTHER_5                      = -1650051,

    // spells
    SPELL_ARGENT_GET_PLAYER_COUNT           = 66986,
    SPELL_ARGENT_SUMMON_CHAMPION_1          = 66654,
    SPELL_ARGENT_SUMMON_CHAMPION_2          = 66671,
    SPELL_ARGENT_SUMMON_CHAMPION_3          = 66673,
    SPELL_ARGENT_SUMMON_CHAMPION_WAVE       = 67295,
    SPELL_ARGENT_SUMMON_BOSS_4              = 67396,
    SPELL_ARGENT_HERALD_FEIGN_DEATH         = 66804,

    // Arena event spells - not used for the moment
    // SPELL_SPECTATOR_FORCE_CHANT          = 66354,
    // SPELL_SPECTATOR_FX_CHANT             = 66677,
};

static const DialogueEntryTwoSide aTocDialogues[] =
{
    // Grand Champions intro
    {TYPE_ARENA_CHALLENGE,          0,                   0, 0, 1000},
    {SAY_HERALD_HORDE_CHALLENGE,    NPC_ARELAS_BRIGHTSTAR,  SAY_HERALD_ALLIANCE_CHALLENGE, NPC_JAEREN_SUNSWORN,    5000},
    {SAY_TIRION_CHALLENGE_WELCOME,  NPC_TIRION_FORDRING, 0, 0, 6000},
    {SAY_TIRION_FIRST_CHALLENGE,    NPC_TIRION_FORDRING, 0, 0, 3000},
    {NPC_TIRION_FORDRING,           0,                   0, 0, 0},
    // Grand Champions complete
    {NPC_ARELAS_BRIGHTSTAR,         0,                   0, 0, 7000},
    {SAY_TIRION_ARGENT_CHAMPION,    NPC_TIRION_FORDRING, 0, 0, 0},
    {0, 0, 0, 0, 0}
};

instance_trial_of_the_champion::instance_trial_of_the_champion(Map* pMap) : ScriptedInstance(pMap), DialogueHelper(aTocDialogues),
    m_uiTeam(TEAM_NONE),
    m_uiHeraldEntry(0),
    m_uiGrandChampionEntry(0),
    m_uiIntroTimer(0),
    m_uiIntroStage(0),
    m_uiArenaStage(0),
    m_uiGateResetTimer(0),
    m_uiChampionsCount(0),
    m_uiChampionsTimer(0),
    m_bSkipIntro(false)
{
    Initialize();
}

void instance_trial_of_the_champion::Initialize()
{
    memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));
    InitializeDialogueHelper(this);

    m_vAllianceTriggersGuids.resize(MAX_CHAMPIONS_AVAILABLE);
    m_vHordeTriggersGuids.resize(MAX_CHAMPIONS_AVAILABLE);
}

void instance_trial_of_the_champion::OnPlayerEnter(Player* pPlayer)
{
    if (!m_uiTeam)
    {
        m_uiTeam = pPlayer->GetTeam();
        SetDialogueSide(m_uiTeam == ALLIANCE);

        m_uiHeraldEntry = m_uiTeam == ALLIANCE ? NPC_ARELAS_BRIGHTSTAR : NPC_JAEREN_SUNSWORN;

        // set a random grand champion
        m_uiGrandChampionEntry = urand(0, 1) ? NPC_EADRIC : NPC_PALETRESS;

        if (m_vChampionsIndex.empty())
        {
            m_vChampionsIndex.resize(MAX_CHAMPIONS_AVAILABLE);

            // fill vector array with indexes from creature array
            for (uint8 i = 0; i < MAX_CHAMPIONS_AVAILABLE; ++i)
                m_vChampionsIndex[i] = i;

            // set a random champion list
            std::random_shuffle(m_vChampionsIndex.begin(), m_vChampionsIndex.end());
        }
    }

    DoSummonHeraldIfNeeded(pPlayer);
}

void instance_trial_of_the_champion::OnCreatureCreate(Creature* pCreature)
{
    switch (pCreature->GetEntry())
    {
        case NPC_JAEREN_SUNSWORN:
        case NPC_ARELAS_BRIGHTSTAR:
        case NPC_TIRION_FORDRING:
        case NPC_VARIAN_WRYNN:
        case NPC_THRALL:
        case NPC_GARROSH:
        case NPC_ALLIANCE_WARRIOR:
        case NPC_ALLIANCE_MAGE:
        case NPC_ALLIANCE_SHAMAN:
        case NPC_ALLIANCE_HUNTER:
        case NPC_ALLIANCE_ROGUE:
        case NPC_HORDE_WARRIOR:
        case NPC_HORDE_MAGE:
        case NPC_HORDE_SHAMAN:
        case NPC_HORDE_HUNTER:
        case NPC_HORDE_ROGUE:
        case NPC_EADRIC:
        case NPC_PALETRESS:
        case NPC_WORLD_TRIGGER:
        case NPC_SPECTATOR_HUMAN:
        case NPC_SPECTATOR_ORC:
        case NPC_SPECTATOR_TROLL:
        case NPC_SPECTATOR_TAUREN:
        case NPC_SPECTATOR_BLOOD_ELF:
        case NPC_SPECTATOR_UNDEAD:
        case NPC_SPECTATOR_DWARF:
        case NPC_SPECTATOR_DRAENEI:
        case NPC_SPECTATOR_NIGHT_ELF:
        case NPC_SPECTATOR_GNOME:
            break;
        case NPC_SPECTATOR_GENERIC:
            // alliance side
            if (pCreature->GetPositionX() > 775.0f)
            {
                // night elf
                if (pCreature->GetPositionY() > 650.0f)
                    m_vAllianceTriggersGuids[3] = pCreature->GetObjectGuid();
                // gnome
                else if (pCreature->GetPositionY() > 630.0f)
                    m_vAllianceTriggersGuids[1] = pCreature->GetObjectGuid();
                // human
                else if (pCreature->GetPositionY() > 615.0f)
                    m_vAllianceTriggersGuids[0] = pCreature->GetObjectGuid();
                // dwarf
                else if (pCreature->GetPositionY() > 595.0f)
                    m_vAllianceTriggersGuids[4] = pCreature->GetObjectGuid();
                // draenei
                else if (pCreature->GetPositionY() > 580.0f)
                    m_vAllianceTriggersGuids[2] = pCreature->GetObjectGuid();
            }
            // horde side
            else if (pCreature->GetPositionX() < 715.0f)
            {
                // undead
                if (pCreature->GetPositionY() > 650.0f)
                    m_vHordeTriggersGuids[4] = pCreature->GetObjectGuid();
                // blood elf
                else if (pCreature->GetPositionY() > 630.0f)
                    m_vHordeTriggersGuids[1] = pCreature->GetObjectGuid();
                // orc
                else if (pCreature->GetPositionY() > 615.0f)
                    m_vHordeTriggersGuids[0] = pCreature->GetObjectGuid();
                // troll
                else if (pCreature->GetPositionY() > 595.0f)
                    m_vHordeTriggersGuids[3] = pCreature->GetObjectGuid();
                // tauren
                else if (pCreature->GetPositionY() > 580.0f)
                    m_vHordeTriggersGuids[2] = pCreature->GetObjectGuid();
            }
            return;
        case NPC_WARHORSE_ALLIANCE:
        case NPC_WARHORSE_HORDE:
        case NPC_BATTLEWORG_ALLIANCE:
        case NPC_BATTLEWORG_HORDE:
            m_lArenaMountsGuids.push_back(pCreature->GetObjectGuid());
            return;
        default:
            return;
    }

    m_mNpcEntryGuidStore[pCreature->GetEntry()] = pCreature->GetObjectGuid();
}

void instance_trial_of_the_champion::OnObjectCreate(GameObject* pGo)
{
    switch (pGo->GetEntry())
    {
        case GO_MAIN_GATE:
        case GO_NORTH_GATE:
        case GO_CHAMPIONS_LOOT:
        case GO_CHAMPIONS_LOOT_H:
        case GO_EADRIC_LOOT:
        case GO_EADRIC_LOOT_H:
        case GO_PALETRESS_LOOT:
        case GO_PALETRESS_LOOT_H:
            break;
        default:
            return;
    }

    m_mGoEntryGuidStore[pGo->GetEntry()] = pGo->GetObjectGuid();
}

void instance_trial_of_the_champion::SetData(uint32 uiType, uint32 uiData)
{
    switch (uiType)
    {
        case TYPE_GRAND_CHAMPIONS:
            // no double count
            if (m_auiEncounter[uiType] == uiData)
                return;

            m_auiEncounter[uiType] = uiData;
            DoUseDoorOrButton(GO_NORTH_GATE);
            if (uiData == DONE)
            {
                DoRespawnGameObject(instance->IsRegularDifficulty() ? GO_CHAMPIONS_LOOT : GO_CHAMPIONS_LOOT_H, 30 * MINUTE);

                // start delayed dialogue
                StartNextDialogueText(NPC_ARELAS_BRIGHTSTAR);

                // move the herald back to center
                if (Creature* pHerald = GetSingleCreatureFromStorage(m_uiHeraldEntry))
                {
                    pHerald->GetMotionMaster()->Clear();
                    pHerald->GetMotionMaster()->MovePoint(0, aHeraldPositions[2][0], aHeraldPositions[2][1], aHeraldPositions[2][2]);
                    pHerald->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                }

                DoSendChampionsToExit();
            }
            break;
        case TYPE_ARGENT_CHAMPION:
            m_auiEncounter[uiType] = uiData;
            if (uiData == DONE)
            {
                if (m_uiGrandChampionEntry == NPC_EADRIC)
                    DoRespawnGameObject(instance->IsRegularDifficulty() ? GO_EADRIC_LOOT : GO_EADRIC_LOOT_H, 30 * MINUTE);
                else if (m_uiGrandChampionEntry == NPC_PALETRESS)
                    DoRespawnGameObject(instance->IsRegularDifficulty() ? GO_PALETRESS_LOOT : GO_PALETRESS_LOOT_H, 30 * MINUTE);
            }
            break;
        case TYPE_BLACK_KNIGHT:
            m_auiEncounter[uiType] = uiData;
            break;
        case TYPE_ARENA_CHALLENGE:
            m_auiEncounter[uiType] = uiData;
            if (uiData == IN_PROGRESS)
            {
                // start arena challenge
                m_uiArenaStage = 0;
                DoSendNextArenaWave();
            }
            else if (uiData == DONE)
            {
                // count the champions that reach the exit
                ++m_uiChampionsCount;

                if (m_uiChampionsCount == MAX_CHAMPIONS_ARENA)
                {
                    // start grand champions challenge (without mount)
                    m_uiChampionsCount = 0;
                    m_uiChampionsTimer = 5000;

                    // despawn vehicle mounts
                    for (GuidList::const_iterator itr = m_lArenaMountsGuids.begin(); itr != m_lArenaMountsGuids.end(); ++itr)
                    {
                        if (Creature* pMount = instance->GetCreature(*itr))
                            pMount->ForcedDespawn();
                    }
                    m_lArenaMountsGuids.clear();
                }
            }
            else if (uiData == FAIL)
            {
                DoCleanupArenaOnWipe();
                SetData(TYPE_ARENA_CHALLENGE, NOT_STARTED);
            }
            else if (uiData == SPECIAL)
                DoSendChampionsToExit();
            return;
        default:
            script_error_log("Instance Trial of The Champion: ERROR SetData = %u for type %u does not exist/not implemented.", uiType, uiData);
            return;
    }

    if (uiData == DONE)
    {
        OUT_SAVE_INST_DATA;

        std::ostringstream saveStream;
        saveStream << m_auiEncounter[0] << " " << m_auiEncounter[1] << " " << m_auiEncounter[2];

        m_strInstData = saveStream.str();

        SaveToDB();
        OUT_SAVE_INST_DATA_COMPLETE;
    }
}

uint32 instance_trial_of_the_champion::GetData(uint32 uiType) const
{
    if (uiType < MAX_ENCOUNTER)
        return m_auiEncounter[uiType];

    return 0;
}

void instance_trial_of_the_champion::Load(const char* chrIn)
{
    if (!chrIn)
    {
        OUT_LOAD_INST_DATA_FAIL;
        return;
    }

    OUT_LOAD_INST_DATA(chrIn);

    std::istringstream loadStream(chrIn);
    loadStream >> m_auiEncounter[0] >> m_auiEncounter[1] >> m_auiEncounter[2];

    for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
    {
        if (m_auiEncounter[i] == IN_PROGRESS)
            m_auiEncounter[i] = NOT_STARTED;
    }

    OUT_LOAD_INST_DATA_COMPLETE;
}

void instance_trial_of_the_champion::OnCreatureDeath(Creature* pCreature)
{
    switch (pCreature->GetEntry())
    {
        case NPC_ALLIANCE_WARRIOR_CHAMPION:
        case NPC_ALLIANCE_MAGE_CHAMPION:
        case NPC_ALLIANCE_SHAMAN_CHAMPION:
        case NPC_ALLIANCE_HUNTER_CHAMPION:
        case NPC_ALLIANCE_ROGUE_CHAMPION:
        case NPC_HORDE_WARRIOR_CHAMPION:
        case NPC_HORDE_MAGE_CHAMPION:
        case NPC_HORDE_SHAMAN_CHAMPION:
        case NPC_HORDE_HUNTER_CHAMPION:
        case NPC_HORDE_ROGUE_CHAMPION:
            // handle champion trash mob kill
            if (m_sArenaHelpersGuids[m_uiArenaStage].find(pCreature->GetObjectGuid()) != m_sArenaHelpersGuids[m_uiArenaStage].end())
            {
                m_sArenaHelpersGuids[m_uiArenaStage].erase(pCreature->GetObjectGuid());

                // send next arena wave if cleared
                if (m_sArenaHelpersGuids[m_uiArenaStage].empty())
                {
                    ++m_uiArenaStage;
                    DoSendNextArenaWave();
                }
            }
            break;
    }
}

void instance_trial_of_the_champion::OnCreatureEvade(Creature* pCreature)
{
    switch (pCreature->GetEntry())
    {
        case NPC_ALLIANCE_WARRIOR_CHAMPION:
        case NPC_ALLIANCE_MAGE_CHAMPION:
        case NPC_ALLIANCE_SHAMAN_CHAMPION:
        case NPC_ALLIANCE_HUNTER_CHAMPION:
        case NPC_ALLIANCE_ROGUE_CHAMPION:
        case NPC_HORDE_WARRIOR_CHAMPION:
        case NPC_HORDE_MAGE_CHAMPION:
        case NPC_HORDE_SHAMAN_CHAMPION:
        case NPC_HORDE_HUNTER_CHAMPION:
        case NPC_HORDE_ROGUE_CHAMPION:
        case NPC_ALLIANCE_WARRIOR:
        case NPC_ALLIANCE_MAGE:
        case NPC_ALLIANCE_SHAMAN:
        case NPC_ALLIANCE_HUNTER:
        case NPC_ALLIANCE_ROGUE:
        case NPC_HORDE_WARRIOR:
        case NPC_HORDE_MAGE:
        case NPC_HORDE_SHAMAN:
        case NPC_HORDE_HUNTER:
        case NPC_HORDE_ROGUE:
            if (GetData(TYPE_ARENA_CHALLENGE) == IN_PROGRESS)
                SetData(TYPE_ARENA_CHALLENGE, FAIL);
            break;
    }
}

// Function that returns the arena challenge status
bool instance_trial_of_the_champion::IsArenaChallengeComplete(uint32 uiType)
{
    // check stand state of each champion based on the type of the encounter
    uint8 uiStandState = 0;

    if (uiType == TYPE_ARENA_CHALLENGE)
    {
        // if this was already marked as complete return true
        if (GetData(TYPE_ARENA_CHALLENGE) == SPECIAL || GetData(TYPE_ARENA_CHALLENGE) == DONE)
            return true;

        uiStandState = UNIT_STAND_STATE_DEAD;
    }
    else if (uiType == TYPE_GRAND_CHAMPIONS)
        uiStandState = UNIT_STAND_STATE_KNEEL;

    // check if all champions are defeated
    for (uint8 i = 0; i < MAX_CHAMPIONS_ARENA; ++i)
    {
        if (Creature* pChampion = instance->GetCreature(m_ArenaChampionsGuids[i]))
        {
            if (pChampion->getStandState() != uiStandState)
                return false;
        }
    }

    return true;
}

// Function that summons herald and vehicle mounts if required
void instance_trial_of_the_champion::DoSummonHeraldIfNeeded(Unit* pSummoner)
{
    if (!pSummoner)
        return;

    if (GetSingleCreatureFromStorage(m_uiHeraldEntry, true))
        return;

    pSummoner->SummonCreature(m_uiHeraldEntry, aHeraldPositions[0][0], aHeraldPositions[0][1], aHeraldPositions[0][2], aHeraldPositions[0][3], TEMPSUMMON_DEAD_DESPAWN, 0);

    // summon champion mounts if required
    if (GetData(TYPE_GRAND_CHAMPIONS) != DONE)
    {
        for (uint8 i = 0; i < MAX_CHAMPIONS_MOUNTS; ++i)
            pSummoner->SummonCreature(m_uiTeam == ALLIANCE ? aTrialChampionsMounts[i].uiEntryAlliance : aTrialChampionsMounts[i].uiEntryHorde, aTrialChampionsMounts[i].fX, aTrialChampionsMounts[i].fY, aTrialChampionsMounts[i].fZ, aTrialChampionsMounts[i].fO, TEMPSUMMON_DEAD_DESPAWN, 0);
    }
}

// Function that sends the champions and trash mobs into to fight in the arena
void instance_trial_of_the_champion::DoSendNextArenaWave()
{
    // center trigger for reference
    Creature* pCenterTrigger = GetSingleCreatureFromStorage(NPC_WORLD_TRIGGER);
    if (!pCenterTrigger)
        return;

    float fX, fY, fZ;

    // trash waves cleaned - send the summoned champions to the center
    if (m_uiArenaStage == MAX_CHAMPIONS_ARENA)
    {
        for (uint8 i = 0; i < MAX_CHAMPIONS_ARENA; ++i)
        {
            // move mounts to center
            if (Creature* pMount = instance->GetCreature(m_ArenaMountsGuids[i]))
            {
                pMount->SetWalk(false);
                pCenterTrigger->GetContactPoint(pMount, fX, fY, fZ, 2 * INTERACTION_DISTANCE);
                pMount->GetMotionMaster()->MovePoint(POINT_ID_COMBAT, fX, fY, fZ);
            }

            // set champions to attack
            if (Creature* pChampion = instance->GetCreature(m_ArenaChampionsGuids[i]))
                pChampion->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_OOC_NOT_ATTACKABLE);
        }
    }
    // send trash waves of champions in the arena
    else
    {
        for (GuidSet::const_iterator itr = m_sArenaHelpersGuids[m_uiArenaStage].begin(); itr != m_sArenaHelpersGuids[m_uiArenaStage].end(); ++itr)
        {
            if (Creature* pHelper = instance->GetCreature(*itr))
            {
                pHelper->SetWalk(false);
                pCenterTrigger->GetContactPoint(pHelper, fX, fY, fZ, 2 * INTERACTION_DISTANCE);
                pHelper->GetMotionMaster()->MovePoint(POINT_ID_COMBAT, fX, fY, fZ);
                pHelper->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_OOC_NOT_ATTACKABLE | UNIT_FLAG_PASSIVE);
            }
        }
    }
}

// Function that cleans the arena on wipe
void instance_trial_of_the_champion::DoCleanupArenaOnWipe()
{
    // cleanup arena encounter
    for (uint8 i = 0; i < MAX_CHAMPIONS_ARENA; ++i)
    {
        if (Creature* pMount = instance->GetCreature(m_ArenaMountsGuids[i]))
            pMount->ForcedDespawn();

        if (Creature* pChampion = instance->GetCreature(m_ArenaChampionsGuids[i]))
            pChampion->ForcedDespawn();

        for (GuidSet::const_iterator itr = m_sArenaHelpersGuids[i].begin(); itr != m_sArenaHelpersGuids[i].end(); ++itr)
        {
            if (Creature* pHelper = instance->GetCreature(*itr))
                pHelper->ForcedDespawn();
        }

        m_sArenaHelpersGuids[i].clear();
    }

    // reset herald
    if (Creature* pHerald = GetSingleCreatureFromStorage(m_uiHeraldEntry))
    {
        pHerald->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
        pHerald->GetMotionMaster()->MoveTargetedHome();
    }
}

// Function that prepares the Grand Champions encounter (long and short intro)
void instance_trial_of_the_champion::DoPrepareChampions(bool bSkipIntro)
{
    m_bSkipIntro = bSkipIntro;

    // long intro
    if (!bSkipIntro)
        StartNextDialogueText(TYPE_ARENA_CHALLENGE);
    // short intro
    else
    {
        StartNextDialogueText(SAY_TIRION_CHALLENGE_WELCOME);

        // move the herald to the gate
        if (Creature* pHerald = GetSingleCreatureFromStorage(m_uiHeraldEntry))
            pHerald->GetMotionMaster()->MovePoint(0, aHeraldPositions[1][0], aHeraldPositions[1][1], aHeraldPositions[1][2]);
    }
}

// Function that sends the champion to intro home location
void instance_trial_of_the_champion::MoveChampionToHome(Creature* pChampion)
{
    uint8 uiIndex = m_vChampionsIndex[m_uiIntroStage - 1];

    // get the corresponding trigger
    Creature* pTrigger = instance->GetCreature(m_uiTeam == ALLIANCE ? m_vHordeTriggersGuids[uiIndex] : m_vAllianceTriggersGuids[uiIndex]);
    if (!pTrigger)
        return;

    // move to the right position
    pChampion->SetWalk(true);
    pChampion->GetMotionMaster()->MovePoint(POINT_ID_HOME, pTrigger->GetPositionX(), pTrigger->GetPositionY(), pTrigger->GetPositionZ());

    if (m_uiIntroStage == MAX_CHAMPIONS_ARENA)
    {
        // move the herald to the gate
        if (Creature* pHerald = GetSingleCreatureFromStorage(m_uiHeraldEntry))
            pHerald->GetMotionMaster()->MovePoint(0, aHeraldPositions[1][0], aHeraldPositions[1][1], aHeraldPositions[1][2]);
    }
}

// Function that handles instance behavior when champion reaches intro home position
// This sets the trash mobs to the right points and sends the next champion for intro
void instance_trial_of_the_champion::InformChampionReachHome()
{
    uint8 uiIndex = m_vChampionsIndex[m_uiIntroStage - 1];

    // get the corresponding trigger
    Creature* pTrigger = instance->GetCreature(m_uiTeam == ALLIANCE ? m_vHordeTriggersGuids[uiIndex] : m_vAllianceTriggersGuids[uiIndex]);
    if (!pTrigger)
        return;

    // center trigger for reference
    Creature* pCenterTrigger = GetSingleCreatureFromStorage(NPC_WORLD_TRIGGER);
    if (!pCenterTrigger)
        return;

    float fX, fY, fZ;
    uint8 j = 0;

    // move helpers to the right point
    for (GuidSet::const_iterator itr = m_sArenaHelpersGuids[m_uiIntroStage - 1].begin(); itr != m_sArenaHelpersGuids[m_uiIntroStage - 1].end(); ++itr)
    {
        if (Creature* pHelper = instance->GetCreature(*itr))
        {
            pTrigger->GetNearPoint(pTrigger, fX, fY, fZ, 0, 5.0f, pTrigger->GetAngle(pCenterTrigger) - (M_PI_F * 0.25f) + j * (M_PI_F * 0.25f));
            pHelper->GetMotionMaster()->Clear();
            pHelper->GetMotionMaster()->MovePoint(POINT_ID_HOME, fX, fY, fZ);
            ++j;
        }
    }

    // set timer
    if (m_uiIntroStage == MAX_CHAMPIONS_ARENA)
        m_uiIntroTimer = 5000;
    else
        m_uiIntroTimer = 2000;
}

// Function that will send all the champions to the get for exit
void instance_trial_of_the_champion::DoSendChampionsToExit()
{
    // move the champions to the gate
    for (uint8 i = 0; i < MAX_CHAMPIONS_ARENA; ++i)
    {
        if (Creature* pChampion = instance->GetCreature(m_ArenaChampionsGuids[i]))
        {
            pChampion->SetWalk(true);
            pChampion->SetStandState(UNIT_STAND_STATE_STAND);
            pChampion->GetMotionMaster()->MovePoint(POINT_ID_EXIT, aChampsPositions[0][0], aChampsPositions[0][1], aChampsPositions[0][2]);
        }
    }
}

void instance_trial_of_the_champion::JustDidDialogueStep(int32 iEntry)
{
    switch (iEntry)
    {
        // start arena long intro
        case TYPE_ARENA_CHALLENGE:
            if (Creature* pHerald = GetSingleCreatureFromStorage(m_uiHeraldEntry))
            {
                if (Creature* pTrigger = GetSingleCreatureFromStorage(NPC_WORLD_TRIGGER))
                    pHerald->GetMotionMaster()->MovePoint(0, pTrigger->GetPositionX(), pTrigger->GetPositionY(), pTrigger->GetPositionZ());
            }
            break;
        case SAY_HERALD_ALLIANCE_CHALLENGE:
        case SAY_HERALD_HORDE_CHALLENGE:
            if (Creature* pHerald = GetSingleCreatureFromStorage(m_uiHeraldEntry))
            {
                if (Creature* pTirion = GetSingleCreatureFromStorage(NPC_TIRION_FORDRING))
                    pHerald->SetFacingToObject(pTirion);

                // ToDo: play intro music
            }
            break;
        // complete intro - start arena event
        case NPC_TIRION_FORDRING:
            m_uiIntroStage = 0;
            m_uiIntroTimer = 1000;
            break;
    }
}

void instance_trial_of_the_champion::Update(uint32 uiDiff)
{
    DialogueUpdate(uiDiff);

    if (m_uiIntroTimer)
    {
        if (m_uiIntroTimer <= uiDiff)
        {
            switch (m_uiIntroStage)
            {
                // spawn champions
                case 0:
                case 1:
                case 2:
                {
                    uint8 uiIndex = m_vChampionsIndex[m_uiIntroStage];

                    // summoner (herald)
                    Creature* pHerald = GetSingleCreatureFromStorage(m_uiHeraldEntry);
                    if (!pHerald)
                        return;

                    // center trigger for reference
                    Creature* pCenterTrigger = GetSingleCreatureFromStorage(NPC_WORLD_TRIGGER);
                    if (!pCenterTrigger)
                        return;

                    // short intro
                    if (m_bSkipIntro)
                    {
                        // get the summoning trigger
                        Creature* pTrigger = instance->GetCreature(m_uiTeam == ALLIANCE ? m_vHordeTriggersGuids[uiIndex] : m_vAllianceTriggersGuids[uiIndex]);
                        if (!pTrigger)
                            return;

                        // summon grand champion, mount and emote
                        if (Creature* pChampion = pHerald->SummonCreature(m_uiTeam == ALLIANCE ? aHordeChampions[uiIndex].uiEntry : aAllianceChampions[uiIndex].uiEntry,
                            pTrigger->GetPositionX(), pTrigger->GetPositionY(), pTrigger->GetPositionZ(), pTrigger->GetAngle(pCenterTrigger), TEMPSUMMON_DEAD_DESPAWN, 0))
                        {
                            // handle emote
                            if (Creature* pStalker = GetSingleCreatureFromStorage(m_uiTeam == ALLIANCE ? aHordeChampions[uiIndex].uiCrowdStalker : aAllianceChampions[uiIndex].uiCrowdStalker))
                                DoScriptText(m_uiTeam == ALLIANCE ? aHordeChampions[uiIndex].iEmoteEntry : aAllianceChampions[uiIndex].iEmoteEntry, pStalker, pChampion);

                            // summon champion mount
                            if (Creature* pMount = pChampion->SummonCreature(m_uiTeam == ALLIANCE ? aHordeChampions[uiIndex].uiMount : aAllianceChampions[uiIndex].uiMount,
                                pTrigger->GetPositionX(), pTrigger->GetPositionY(), pTrigger->GetPositionZ(), pTrigger->GetAngle(pCenterTrigger), TEMPSUMMON_DEAD_DESPAWN, 0))
                            {
                                pChampion->CastSpell(pMount, SPELL_RIDE_VEHICLE_HARDCODED, true);

                                // set guid
                                m_ArenaChampionsGuids[m_uiIntroStage] = pChampion->GetObjectGuid();
                                m_ArenaMountsGuids[m_uiIntroStage] = pMount->GetObjectGuid();
                            }
                        }

                        // summon helper champions
                        float fX, fY, fZ;
                        for (uint8 j = 0; j < 3; ++j)
                        {
                            pTrigger->GetNearPoint(pTrigger, fX, fY, fZ, 0, 5.0f, pTrigger->GetAngle(pCenterTrigger) - (M_PI_F * 0.25f) + j * (M_PI_F * 0.25f));
                            if (Creature* pHelper = pHerald->SummonCreature(m_uiTeam == ALLIANCE ? aHordeChampions[uiIndex].uiChampion : aAllianceChampions[uiIndex].uiChampion,
                                fX, fY, fZ, pTrigger->GetAngle(pCenterTrigger), TEMPSUMMON_DEAD_DESPAWN, 0))
                                m_sArenaHelpersGuids[m_uiIntroStage].insert(pHelper->GetObjectGuid());
                        }

                        if (m_uiIntroStage == MAX_CHAMPIONS_ARENA - 1)
                            m_uiIntroTimer = 5000;
                        else
                            m_uiIntroTimer = 2000;
                    }
                    // long intro
                    else
                    {
                        float fX, fY, fZ;
                        DoUseDoorOrButton(GO_MAIN_GATE);
                        m_uiGateResetTimer = 10000;             // ToDo: set this as door reset timer when fixed in core

                        // summon grand champion, mount and emote
                        if (Creature* pChampion = pHerald->SummonCreature(m_uiTeam == ALLIANCE ? aHordeChampions[uiIndex].uiEntry : aAllianceChampions[uiIndex].uiEntry,
                            aIntroPositions[0][0], aIntroPositions[0][1], aIntroPositions[0][2], aIntroPositions[0][3], TEMPSUMMON_DEAD_DESPAWN, 0))
                        {
                            // text
                            DoScriptText(m_uiTeam == ALLIANCE ? aHordeChampions[uiIndex].iYellEntry : aAllianceChampions[uiIndex].iYellEntry, pHerald);
                            pHerald->SetFacingToObject(pChampion);

                            // handle emote
                            if (Creature* pStalker = GetSingleCreatureFromStorage(m_uiTeam == ALLIANCE ? aHordeChampions[uiIndex].uiCrowdStalker : aAllianceChampions[uiIndex].uiCrowdStalker))
                                DoScriptText(m_uiTeam == ALLIANCE ? aHordeChampions[uiIndex].iEmoteEntry : aAllianceChampions[uiIndex].iEmoteEntry, pStalker, pChampion);

                            // summon champion mount
                            if (Creature* pMount = pChampion->SummonCreature(m_uiTeam == ALLIANCE ? aHordeChampions[uiIndex].uiMount : aAllianceChampions[uiIndex].uiMount,
                                aIntroPositions[0][0], aIntroPositions[0][1], aIntroPositions[0][2], aIntroPositions[0][3], TEMPSUMMON_DEAD_DESPAWN, 0))
                            {
                                pChampion->CastSpell(pMount, SPELL_RIDE_VEHICLE_HARDCODED, true);

                                pMount->SetWalk(false);
                                pCenterTrigger->GetContactPoint(pChampion, fX, fY, fZ, 2 * INTERACTION_DISTANCE);
                                pMount->GetMotionMaster()->MovePoint(POINT_ID_CENTER, fX, fY, fZ);

                                // set guid
                                m_ArenaChampionsGuids[m_uiIntroStage] = pChampion->GetObjectGuid();
                                m_ArenaMountsGuids[m_uiIntroStage] = pMount->GetObjectGuid();

                                // summon helper champions
                                for (uint8 j = 0; j < 3; ++j)
                                {
                                    if (Creature* pHelper = pChampion->SummonCreature(m_uiTeam == ALLIANCE ? aHordeChampions[uiIndex].uiChampion : aAllianceChampions[uiIndex].uiChampion,
                                        aIntroPositions[j + 1][0], aIntroPositions[j + 1][1], aIntroPositions[j + 1][2], aIntroPositions[j + 1][3], TEMPSUMMON_DEAD_DESPAWN, 0))
                                    {
                                        pHelper->GetMotionMaster()->MoveFollow(pMount, pHelper->GetDistance(pMount), M_PI_F/2 + pHelper->GetAngle(pMount));
                                        m_sArenaHelpersGuids[m_uiIntroStage].insert(pHelper->GetObjectGuid());
                                    }
                                }
                            }
                        }

                        // stop event; timer in InformChampionReachHome()
                        m_uiIntroTimer = 0;
                    }
                    break;
                }
                // complete intro - start arena challenge
                case 3:
                    if (Creature* pTirion = GetSingleCreatureFromStorage(NPC_TIRION_FORDRING))
                        DoScriptText(SAY_TIRION_CHALLENGE_BEGIN, pTirion);

                    if (Creature* pHerald = GetSingleCreatureFromStorage(m_uiHeraldEntry))
                    {
                        if (Creature* pCenterTrigger = GetSingleCreatureFromStorage(NPC_WORLD_TRIGGER))
                            pHerald->SetFacingToObject(pCenterTrigger);
                    }

                    // start first half of the encounter
                    SetData(TYPE_ARENA_CHALLENGE, IN_PROGRESS);
                    m_uiIntroTimer = 0;
                    break;
            }
            ++m_uiIntroStage;
        }
        else
            m_uiIntroTimer -= uiDiff;
    }

    // ToDo: set this as door reset timer when fixed in core
    if (m_uiGateResetTimer)
    {
        if (m_uiGateResetTimer <= uiDiff)
        {
            DoUseDoorOrButton(GO_MAIN_GATE);
            m_uiGateResetTimer = 0;
        }
        else
            m_uiGateResetTimer -= uiDiff;
    }

    // summon champions for the second part of the encounter
    if (m_uiChampionsTimer)
    {
        if (m_uiChampionsTimer <= uiDiff)
        {
            Creature* pHerald = GetSingleCreatureFromStorage(m_uiHeraldEntry);
            if (!pHerald)
                return;

            uint8 uiIndex = 0;

            for (uint8 i = 0; i < MAX_CHAMPIONS_ARENA; ++i)
            {
                uiIndex = m_vChampionsIndex[i];

                if (Creature* pChampion = pHerald->SummonCreature(m_uiTeam == ALLIANCE ? aHordeChampions[uiIndex].uiEntry : aAllianceChampions[uiIndex].uiEntry,
                    aChampsPositions[i][0], aChampsPositions[i][1], aChampsPositions[i][2], aChampsPositions[i][3], TEMPSUMMON_DEAD_DESPAWN, 0))
                    m_ArenaChampionsGuids[i] = pChampion->GetObjectGuid();
            }
            m_uiChampionsTimer = 0;
        }
        else
            m_uiChampionsTimer -= uiDiff;
    }
}

InstanceData* GetInstanceData_instance_trial_of_the_champion(Map* pMap)
{
    return new instance_trial_of_the_champion(pMap);
}

void AddSC_instance_trial_of_the_champion()
{
    Script* pNewScript;

    pNewScript = new Script;
    pNewScript->Name = "instance_trial_of_the_champion";
    pNewScript->GetInstanceData = &GetInstanceData_instance_trial_of_the_champion;
    pNewScript->RegisterSelf();
}

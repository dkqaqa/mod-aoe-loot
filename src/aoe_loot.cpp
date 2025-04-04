#include "aoe_loot.h"

void AOELootPlayer::OnPlayerLogin(Player* player)
{
    if (sConfigMgr->GetOption<bool>("AOELoot.Enable", true))
    {
        if (sConfigMgr->GetOption<bool>("AOELoot.Message", true))
            ChatHandler(player->GetSession()).PSendSysMessage(AOE_ACORE_STRING_MESSAGE);
    }
}

bool AOELootServer::CanPacketReceive(WorldSession* session, WorldPacket& packet)
{
    if (packet.GetOpcode() == CMSG_LOOT)
    {
        Player* player = session->GetPlayer();

        if (!sConfigMgr->GetOption<bool>("AOELoot.Enable", true))
            return true;

        if (player->GetGroup() && !sConfigMgr->GetOption<bool>("AOELoot.Group", true))
            return true;

        float range = sConfigMgr->GetOption<float>("AOELoot.Range", 55.0);

        std::list<Creature*> lootcreature; player->GetDeadCreatureListInGrid(lootcreature, range);

        ObjectGuid guid; packet >> guid;

        Loot* mainloot = &(player->GetMap()->GetCreature(guid))->loot;

        for (auto itr = lootcreature.begin(); itr != lootcreature.end(); ++itr)
        {
            Creature* creature = *itr;

            // Prevent infiny add items
            if (creature->GetGUID() == guid)
                continue;

            // Prevent steal loot
            if (!player->GetMap()->Instanceable())
                if (!player->isAllowedToLoot(creature))
                    continue;

            // Max 15 items per creature
            if (mainloot->items.size() + mainloot->quest_items.size() > 15)
                break;

            Loot* loot = &(*itr)->loot;

            // FILL QITEMS
            if (!loot->quest_items.empty())
            {
                mainloot->items.insert(mainloot->items.end(), loot->quest_items.begin(), loot->quest_items.end());
                loot->quest_items.clear();
            }

            // FILL GOLD
            if (loot->gold != 0)
            {
                mainloot->gold += loot->gold;
                loot->gold = 0;
            }

            // FILL ITEMS
            if (!loot->items.empty())
            {
                mainloot->items.insert(mainloot->items.end(), loot->items.begin(), loot->items.end());
                loot->items.clear();
            }

            // Set flag for skinning
            if (loot->items.empty() && loot->quest_items.empty())
            {
                creature->AllLootRemovedFromCorpse();
                creature->RemoveDynamicFlag(UNIT_DYNFLAG_LOOTABLE);
            }
        }

        player->SendLoot(guid, LOOT_CORPSE);
        return false;
    }

    return true;
}

/*
 * Dark Souls 3 - Open Server
 * Copyright (C) 2021 Tim Leonard
 *
 * This program is free software; licensed under the MIT license.
 * You should have received a copy of the license along with this program.
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#include "Server/GameService/GameManagers/Logging/LoggingManager.h"
#include "Server/GameService/GameClient.h"
#include "Server/Streams/Frpg2ReliableUdpMessage.h"
#include "Server/Streams/Frpg2ReliableUdpMessageStream.h"

#include "Config/RuntimeConfig.h"
#include "Server/Server.h"
#include "Server/Database/ServerDatabase.h"

#include "Core/Utils/Logging.h"
#include "Core/Utils/Strings.h"

LoggingManager::LoggingManager(Server* InServerInstance)
    : ServerInstance(InServerInstance)
{
}

MessageHandleResult LoggingManager::OnMessageRecieved(GameClient* Client, const Frpg2ReliableUdpMessage& Message)
{
    if (Message.Header.msg_type == Frpg2ReliableUdpMessageType::RequestNotifyProtoBufLog)
    {
        return Handle_RequestNotifyProtoBufLog(Client, Message);
    }
    else if (Message.Header.msg_type == Frpg2ReliableUdpMessageType::RequestNotifyKillEnemy)
    {
        return Handle_RequestNotifyKillEnemy(Client, Message);
    }
    else if (Message.Header.msg_type == Frpg2ReliableUdpMessageType::RequestNotifyDisconnectSession)
    {
        return Handle_RequestNotifyDisconnectSession(Client, Message);
    }
    else if (Message.Header.msg_type == Frpg2ReliableUdpMessageType::RequestNotifyRegisterCharacter)
    {
        return Handle_RequestNotifyRegisterCharacter(Client, Message);
    }
    else if (Message.Header.msg_type == Frpg2ReliableUdpMessageType::RequestNotifyDie)
    {
        return Handle_RequestNotifyDie(Client, Message);
    }
    else if (Message.Header.msg_type == Frpg2ReliableUdpMessageType::RequestNotifyKillBoss)
    {
        return Handle_RequestNotifyKillBoss(Client, Message);
    }
    else if (Message.Header.msg_type == Frpg2ReliableUdpMessageType::RequestNotifyJoinMultiplay)
    {
        return Handle_RequestNotifyJoinMultiplay(Client, Message);
    }
    else if (Message.Header.msg_type == Frpg2ReliableUdpMessageType::RequestNotifyLeaveMultiplay)
    {
        return Handle_RequestNotifyLeaveMultiplay(Client, Message);
    }
    else if (Message.Header.msg_type == Frpg2ReliableUdpMessageType::RequestNotifySummonSignResult)
    {
        return Handle_RequestNotifySummonSignResult(Client, Message);
    }
    else if (Message.Header.msg_type == Frpg2ReliableUdpMessageType::RequestNotifyCreateSignResult)
    {
        return Handle_RequestNotifyCreateSignResult(Client, Message);
    }
    else if (Message.Header.msg_type == Frpg2ReliableUdpMessageType::RequestNotifyBreakInResult)
    {
        return Handle_RequestNotifyBreakInResult(Client, Message);
    }
        
    return MessageHandleResult::Unhandled;
}

MessageHandleResult LoggingManager::Handle_RequestNotifyProtoBufLog(GameClient* Client, const Frpg2ReliableUdpMessage& Message)
{
    ServerDatabase& Database = ServerInstance->GetDatabase();
    PlayerState& Player = Client->GetPlayerState();

    Frpg2RequestMessage::RequestNotifyProtoBufLog* Request = (Frpg2RequestMessage::RequestNotifyProtoBufLog*)Message.Protobuf.get();

    switch (Request->type())
    {
        case Frpg2RequestMessage::LogType::UseMagicLog:         Handle_UseMagicLog(Client, Request);            break;
        case Frpg2RequestMessage::LogType::ActGestureLog:       Handle_ActGestureLog(Client, Request);          break;
        case Frpg2RequestMessage::LogType::UseItemLog:          Handle_UseItemLog(Client, Request);             break;
        case Frpg2RequestMessage::LogType::PurchaseItemLog:     Handle_PurchaseItemLog(Client, Request);        break;
        case Frpg2RequestMessage::LogType::GetItemLog:          Handle_GetItemLog(Client, Request);             break;
        case Frpg2RequestMessage::LogType::DropItemLog:         Handle_DropItemLog(Client, Request);            break;
        case Frpg2RequestMessage::LogType::LeaveItemLog:        Handle_LeaveItemLog(Client, Request);           break;
        case Frpg2RequestMessage::LogType::SaleItemLog:         Handle_SaleItemLog(Client, Request);            break;
        case Frpg2RequestMessage::LogType::StrengthenWeaponLog: Handle_StrengthenWeaponLog(Client, Request);    break;

        // There are other log types, but none we particularly care about handling, so just ignore them.
    }

    Frpg2RequestMessage::EmptyResponse Response;
    if (!Client->MessageStream->Send(&Response, &Message))
    {
        WarningS(Client->GetName().c_str(), "Disconnecting client as failed to send EmptyResponse response.");
        return MessageHandleResult::Error;
    }

    return MessageHandleResult::Handled;
}

void LoggingManager::Handle_UseMagicLog(GameClient* Client, Frpg2RequestMessage::RequestNotifyProtoBufLog* Request)
{
    ServerDatabase& Database = ServerInstance->GetDatabase();
    PlayerState& Player = Client->GetPlayerState();

    FpdLogMessage::UseMagicLog Log;
    if (!Log.ParseFromArray(Request->data().data(), (int)Request->data().size()))
    {
        WarningS(Client->GetName().c_str(), "Failed to parse UseMagicLog.");
        return;
    }

    uint32_t TotalCount = 0;
    for (int i = 0; i < Log.use_magic_info_list_size(); i++)
    {
        const FpdLogMessage::UseMagicLog_Use_magic_info_list& Item = Log.use_magic_info_list(i);

        std::string StatisticKey = StringFormat("Magic/TotalUsed/Id=%u", Item.spell_id());
        Database.AddGlobalStatistic(StatisticKey, Item.count());
        Database.AddPlayerStatistic(StatisticKey, Player.GetPlayerId(), Item.count());

        TotalCount += Item.count();
    }

    std::string TotalStatisticKey = StringFormat("Magic/TotalUsed");
    Database.AddGlobalStatistic(TotalStatisticKey, TotalCount);
    Database.AddPlayerStatistic(TotalStatisticKey, Player.GetPlayerId(), TotalCount);
}

void LoggingManager::Handle_ActGestureLog(GameClient* Client, Frpg2RequestMessage::RequestNotifyProtoBufLog* Request)
{
    ServerDatabase& Database = ServerInstance->GetDatabase();
    PlayerState& Player = Client->GetPlayerState();

    FpdLogMessage::ActGestureLog Log;
    if (!Log.ParseFromArray(Request->data().data(), (int)Request->data().size()))
    {
        WarningS(Client->GetName().c_str(), "Failed to parse ActGestureLog.");
        return;
    }

    uint32_t TotalCount = 0;
    for (int i = 0; i < Log.use_gesture_info_list_size(); i++)
    {
        const FpdLogMessage::ActGestureLog_Use_gesture_info_list& Item = Log.use_gesture_info_list(i);

        std::string StatisticKey = StringFormat("Gesture/TotalUsed/Id=%u", Item.guesture_id());
        Database.AddGlobalStatistic(StatisticKey, Item.count());
        Database.AddPlayerStatistic(StatisticKey, Player.GetPlayerId(), Item.count());

        TotalCount += Item.count();
    }

    std::string TotalStatisticKey = StringFormat("Gesture/TotalUsed");
    Database.AddGlobalStatistic(TotalStatisticKey, TotalCount);
    Database.AddPlayerStatistic(TotalStatisticKey, Player.GetPlayerId(), TotalCount);
}

void LoggingManager::Handle_UseItemLog(GameClient* Client, Frpg2RequestMessage::RequestNotifyProtoBufLog* Request)
{
    ServerDatabase& Database = ServerInstance->GetDatabase();
    PlayerState& Player = Client->GetPlayerState();

    FpdLogMessage::UseItemLog Log;
    if (!Log.ParseFromArray(Request->data().data(), (int)Request->data().size()))
    {
        WarningS(Client->GetName().c_str(), "Failed to parse UseItemLog.");
        return;
    }

    uint32_t TotalCount = 0;
    for (int i = 0; i < Log.use_item_info_list_size(); i++)
    {
        const FpdLogMessage::UseItemLog_Use_item_info_list& Item = Log.use_item_info_list(i);

        std::string StatisticKey = StringFormat("Item/TotalUsed/Id=%u", Item.item_id());
        Database.AddGlobalStatistic(StatisticKey, Item.count());
        Database.AddPlayerStatistic(StatisticKey, Player.GetPlayerId(), Item.count());

        TotalCount += Item.count();
    }

    std::string TotalStatisticKey = StringFormat("Item/TotalUsed");
    Database.AddGlobalStatistic(TotalStatisticKey, TotalCount);
    Database.AddPlayerStatistic(TotalStatisticKey, Player.GetPlayerId(), TotalCount);
}

void LoggingManager::Handle_PurchaseItemLog(GameClient* Client, Frpg2RequestMessage::RequestNotifyProtoBufLog* Request)
{
    ServerDatabase& Database = ServerInstance->GetDatabase();
    PlayerState& Player = Client->GetPlayerState();

    FpdLogMessage::PurchaseItemLog Log;
    if (!Log.ParseFromArray(Request->data().data(), (int)Request->data().size()))
    {
        WarningS(Client->GetName().c_str(), "Failed to parse PurchaseItemLog.");
        return;
    }

    uint32_t TotalCount = 0;
    for (int i = 0; i < Log.purchase_item_info_list_size(); i++)
    {
        const FpdLogMessage::PurchaseItemLog_Purchase_item_info_list& Item = Log.purchase_item_info_list(i);

        std::string StatisticKey = StringFormat("Item/TotalPurchased/Id=%u", Item.item_id());
        Database.AddGlobalStatistic(StatisticKey, Item.count());
        Database.AddPlayerStatistic(StatisticKey, Player.GetPlayerId(), Item.count());

        TotalCount += Item.count();
    }

    std::string TotalStatisticKey = StringFormat("Item/TotalPurchased");
    Database.AddGlobalStatistic(TotalStatisticKey, TotalCount);
    Database.AddPlayerStatistic(TotalStatisticKey, Player.GetPlayerId(), TotalCount);
}

void LoggingManager::Handle_GetItemLog(GameClient* Client, Frpg2RequestMessage::RequestNotifyProtoBufLog* Request)
{
    ServerDatabase& Database = ServerInstance->GetDatabase();
    PlayerState& Player = Client->GetPlayerState();

    FpdLogMessage::GetItemLog Log;
    if (!Log.ParseFromArray(Request->data().data(), (int)Request->data().size()))
    {
        WarningS(Client->GetName().c_str(), "Failed to parse GetItemLog.");
        return;
    }

    uint32_t TotalCount = 0;
    for (int i = 0; i < Log.get_item_info_list_size(); i++)
    {
        const FpdLogMessage::GetItemLog_Get_item_info_list& Item = Log.get_item_info_list(i);

        std::string StatisticKey = StringFormat("Item/TotalRecieved/Id=%u", Item.item_id());
        Database.AddGlobalStatistic(StatisticKey, Item.count());
        Database.AddPlayerStatistic(StatisticKey, Player.GetPlayerId(), Item.count());

        TotalCount += Item.count();
    }

    std::string TotalStatisticKey = StringFormat("Item/TotalRecieved");
    Database.AddGlobalStatistic(TotalStatisticKey, TotalCount);
    Database.AddPlayerStatistic(TotalStatisticKey, Player.GetPlayerId(), TotalCount);
}

void LoggingManager::Handle_DropItemLog(GameClient* Client, Frpg2RequestMessage::RequestNotifyProtoBufLog* Request)
{
    ServerDatabase& Database = ServerInstance->GetDatabase();
    PlayerState& Player = Client->GetPlayerState();

    FpdLogMessage::DropItemLog Log;
    if (!Log.ParseFromArray(Request->data().data(), (int)Request->data().size()))
    {
        WarningS(Client->GetName().c_str(), "Failed to parse DropItemLog.");
        return;
    }

    uint32_t TotalCount = 0;
    for (int i = 0; i < Log.throw_away_item_list_size(); i++)
    {
        const FpdLogMessage::DropItemLog_Throw_away_item_list& Item = Log.throw_away_item_list(i);

        std::string StatisticKey = StringFormat("Item/TotalDropped/Id=%u", Item.item_id());
        Database.AddGlobalStatistic(StatisticKey, Item.count());
        Database.AddPlayerStatistic(StatisticKey, Player.GetPlayerId(), Item.count());

        TotalCount += Item.count();
    }

    std::string TotalStatisticKey = StringFormat("Item/TotalDropped");
    Database.AddGlobalStatistic(TotalStatisticKey, TotalCount);
    Database.AddPlayerStatistic(TotalStatisticKey, Player.GetPlayerId(), TotalCount);
}

void LoggingManager::Handle_LeaveItemLog(GameClient* Client, Frpg2RequestMessage::RequestNotifyProtoBufLog* Request)
{
    ServerDatabase& Database = ServerInstance->GetDatabase();
    PlayerState& Player = Client->GetPlayerState();

    FpdLogMessage::LeaveItemLog Log;
    if (!Log.ParseFromArray(Request->data().data(), (int)Request->data().size()))
    {
        WarningS(Client->GetName().c_str(), "Failed to parse LeaveItemLog.");
        return;
    }

    uint32_t TotalCount = 0;
    for (int i = 0; i < Log.set_item_info_list_size(); i++)
    {
        const FpdLogMessage::LeaveItemLog_Set_item_info_list& Item = Log.set_item_info_list(i);

        std::string StatisticKey = StringFormat("Item/TotalLeft/Id=%u", Item.item_id());
        Database.AddGlobalStatistic(StatisticKey, Item.count());
        Database.AddPlayerStatistic(StatisticKey, Player.GetPlayerId(), Item.count());

        TotalCount += Item.count();
    }

    std::string TotalStatisticKey = StringFormat("Item/TotalLeft");
    Database.AddGlobalStatistic(TotalStatisticKey, TotalCount);
    Database.AddPlayerStatistic(TotalStatisticKey, Player.GetPlayerId(), TotalCount);
}

void LoggingManager::Handle_SaleItemLog(GameClient* Client, Frpg2RequestMessage::RequestNotifyProtoBufLog* Request)
{
    ServerDatabase& Database = ServerInstance->GetDatabase();
    PlayerState& Player = Client->GetPlayerState();

    FpdLogMessage::SaleItemLog Log;
    if (!Log.ParseFromArray(Request->data().data(), (int)Request->data().size()))
    {
        WarningS(Client->GetName().c_str(), "Failed to parse SaleItemLog.");
        return;
    }

    uint32_t TotalCount = 0;
    for (int i = 0; i < Log.sale_item_info_list_size(); i++)
    {
        const FpdLogMessage::SaleItemLog_Sale_item_info_list& Item = Log.sale_item_info_list(i);

        std::string StatisticKey = StringFormat("Item/TotalSold/Id=%u", Item.item_id());
        Database.AddGlobalStatistic(StatisticKey, Item.count());
        Database.AddPlayerStatistic(StatisticKey, Player.GetPlayerId(), Item.count());

        TotalCount += Item.count();
    }

    std::string TotalStatisticKey = StringFormat("Item/TotalSold");
    Database.AddGlobalStatistic(TotalStatisticKey, TotalCount);
    Database.AddPlayerStatistic(TotalStatisticKey, Player.GetPlayerId(), TotalCount);
}

void LoggingManager::Handle_StrengthenWeaponLog(GameClient* Client, Frpg2RequestMessage::RequestNotifyProtoBufLog* Request)
{
    ServerDatabase& Database = ServerInstance->GetDatabase();
    PlayerState& Player = Client->GetPlayerState();

    FpdLogMessage::StrengthenWeaponLog Log;
    if (!Log.ParseFromArray(Request->data().data(), (int)Request->data().size()))
    {
        WarningS(Client->GetName().c_str(), "Failed to parse StrengthenWeaponLog.");
        return;
    }

    uint32_t TotalCount = 0;
    for (int i = 0; i < Log.strengthen_weapon_info_list_size(); i++)
    {
        const FpdLogMessage::StrengthenWeaponLog_Strengthen_weapon_info_list& Item = Log.strengthen_weapon_info_list(i);

        std::string StatisticKey = StringFormat("Item/TotalUpgraded/Id=%u", Item.from_item_id());
        Database.AddGlobalStatistic(StatisticKey, 1);
        Database.AddPlayerStatistic(StatisticKey, Player.GetPlayerId(), 1);

        TotalCount += 1;
    }

    std::string TotalStatisticKey = StringFormat("Item/TotalUpgraded");
    Database.AddGlobalStatistic(TotalStatisticKey, TotalCount);
    Database.AddPlayerStatistic(TotalStatisticKey, Player.GetPlayerId(), TotalCount);
}

MessageHandleResult LoggingManager::Handle_RequestNotifyKillEnemy(GameClient* Client, const Frpg2ReliableUdpMessage& Message)
{
    ServerDatabase& Database = ServerInstance->GetDatabase();
    PlayerState& Player = Client->GetPlayerState();

    Frpg2RequestMessage::RequestNotifyKillEnemy* Request = (Frpg2RequestMessage::RequestNotifyKillEnemy*)Message.Protobuf.get();

    int EnemyCount = 0;
    for (int i = 0; i < Request->enemys_size(); i++)
    {
        const Frpg2RequestMessage::KillEnemyInfo& EnemyInfo = Request->enemys(i);

        std::string StatisticKey = StringFormat("Enemies/TotalKilled/Id=%u", EnemyInfo.enemy_type_id());
        Database.AddGlobalStatistic(StatisticKey, EnemyInfo.count());
        Database.AddPlayerStatistic(StatisticKey, Player.GetPlayerId(), EnemyInfo.count());

        EnemyCount += EnemyInfo.count();
    }

    std::string TotalStatisticKey = StringFormat("Enemies/TotalKilled");
    Database.AddGlobalStatistic(TotalStatisticKey, EnemyCount);
    Database.AddPlayerStatistic(TotalStatisticKey, Player.GetPlayerId(), EnemyCount);
    
    Frpg2RequestMessage::EmptyResponse Response;
    if (!Client->MessageStream->Send(&Response, &Message))
    {
        WarningS(Client->GetName().c_str(), "Disconnecting client as failed to send EmptyResponse response.");
        return MessageHandleResult::Error;
    }

    return MessageHandleResult::Handled;
}

MessageHandleResult LoggingManager::Handle_RequestNotifyDisconnectSession(GameClient* Client, const Frpg2ReliableUdpMessage& Message)
{
    Frpg2RequestMessage::RequestNotifyDisconnectSession* Request = (Frpg2RequestMessage::RequestNotifyDisconnectSession*)Message.Protobuf.get();

    // Note: I don't think we really care about this log.

    Frpg2RequestMessage::EmptyResponse Response;
    if (!Client->MessageStream->Send(&Response, &Message))
    {
        WarningS(Client->GetName().c_str(), "Disconnecting client as failed to send EmptyResponse response.");
        return MessageHandleResult::Error;
    }

    return MessageHandleResult::Handled;
}

MessageHandleResult LoggingManager::Handle_RequestNotifyRegisterCharacter(GameClient* Client, const Frpg2ReliableUdpMessage& Message)
{
    ServerDatabase& Database = ServerInstance->GetDatabase();
    PlayerState& Player = Client->GetPlayerState();

    Frpg2RequestMessage::RequestNotifyRegisterCharacter* Request = (Frpg2RequestMessage::RequestNotifyRegisterCharacter*)Message.Protobuf.get();

    std::string TotalStatisticKey = StringFormat("Player/TotalRegisteredCharacters");
    Database.AddGlobalStatistic(TotalStatisticKey, 1);
    Database.AddPlayerStatistic(TotalStatisticKey, Player.GetPlayerId(), 1);

    Frpg2RequestMessage::EmptyResponse Response;
    if (!Client->MessageStream->Send(&Response, &Message))
    {
        WarningS(Client->GetName().c_str(), "Disconnecting client as failed to send EmptyResponse response.");
        return MessageHandleResult::Error;
    }

    return MessageHandleResult::Handled;
}

MessageHandleResult LoggingManager::Handle_RequestNotifyDie(GameClient* Client, const Frpg2ReliableUdpMessage& Message)
{
    ServerDatabase& Database = ServerInstance->GetDatabase();
    PlayerState& Player = Client->GetPlayerState();

    Frpg2RequestMessage::RequestNotifyDie* Request = (Frpg2RequestMessage::RequestNotifyDie*)Message.Protobuf.get();

    std::string TypeStatisticKey = StringFormat("Player/TotalDeaths/Cause=%u", (uint32_t)Request->cause_of_death());
    Database.AddGlobalStatistic(TypeStatisticKey, 1);
    Database.AddPlayerStatistic(TypeStatisticKey, Player.GetPlayerId(), 1);

    std::string TotalStatisticKey = StringFormat("Player/TotalDeaths");
    Database.AddGlobalStatistic(TotalStatisticKey, 1);
    Database.AddPlayerStatistic(TotalStatisticKey, Player.GetPlayerId(), 1);

    Frpg2RequestMessage::EmptyResponse Response;
    if (!Client->MessageStream->Send(&Response, &Message))
    {
        WarningS(Client->GetName().c_str(), "Disconnecting client as failed to send EmptyResponse response.");
        return MessageHandleResult::Error;
    }

    return MessageHandleResult::Handled;
}

MessageHandleResult LoggingManager::Handle_RequestNotifyKillBoss(GameClient* Client, const Frpg2ReliableUdpMessage& Message)
{
    Frpg2RequestMessage::RequestNotifyKillBoss* Request = (Frpg2RequestMessage::RequestNotifyKillBoss*)Message.Protobuf.get();

    // Note: I don't think we really care about this log. We get the death information from KillEnemy notification.

    Frpg2RequestMessage::EmptyResponse Response;
    if (!Client->MessageStream->Send(&Response, &Message))
    {
        WarningS(Client->GetName().c_str(), "Disconnecting client as failed to send EmptyResponse response.");
        return MessageHandleResult::Error;
    }

    return MessageHandleResult::Handled;
}

MessageHandleResult LoggingManager::Handle_RequestNotifyJoinMultiplay(GameClient* Client, const Frpg2ReliableUdpMessage& Message)
{
    Frpg2RequestMessage::RequestNotifyJoinMultiplay* Request = (Frpg2RequestMessage::RequestNotifyJoinMultiplay*)Message.Protobuf.get();

    // Note: I don't think we really care about this log. We get most of this info from LeaveMultiplay anyway.

    Frpg2RequestMessage::EmptyResponse Response;
    if (!Client->MessageStream->Send(&Response, &Message))
    {
        WarningS(Client->GetName().c_str(), "Disconnecting client as failed to send EmptyResponse response.");
        return MessageHandleResult::Error;
    }

    return MessageHandleResult::Handled;
}

MessageHandleResult LoggingManager::Handle_RequestNotifyLeaveMultiplay(GameClient* Client, const Frpg2ReliableUdpMessage& Message)
{
    ServerDatabase& Database = ServerInstance->GetDatabase();
    PlayerState& Player = Client->GetPlayerState();

    Frpg2RequestMessage::RequestNotifyLeaveMultiplay* Request = (Frpg2RequestMessage::RequestNotifyLeaveMultiplay*)Message.Protobuf.get();

    std::string TypeStatisticKey = StringFormat("Player/TotalMultiplaySessions");
    Database.AddGlobalStatistic(TypeStatisticKey, 1);
    Database.AddPlayerStatistic(TypeStatisticKey, Player.GetPlayerId(), 1);

    for (int i = 0; i < Request->party_member_info_size(); i++)
    {        
        const Frpg2RequestMessage::PartyMemberInfo& Info = Request->party_member_info(i);
        std::string TypeStatisticKey = StringFormat("Player/TotalMultiplaySessions/PartyPlayerId=%u", Info.player_id());
        Database.AddPlayerStatistic(TypeStatisticKey, Player.GetPlayerId(), 1);
    }

    Frpg2RequestMessage::EmptyResponse Response;
    if (!Client->MessageStream->Send(&Response, &Message))
    {
        WarningS(Client->GetName().c_str(), "Disconnecting client as failed to send EmptyResponse response.");
        return MessageHandleResult::Error;
    }

    return MessageHandleResult::Handled;
}

MessageHandleResult LoggingManager::Handle_RequestNotifySummonSignResult(GameClient* Client, const Frpg2ReliableUdpMessage& Message)
{
    Frpg2RequestMessage::RequestNotifySummonSignResult* Request = (Frpg2RequestMessage::RequestNotifySummonSignResult*)Message.Protobuf.get();

    // Note: I don't think we really care about this log. We get most of this during the summon flow.

    Frpg2RequestMessage::EmptyResponse Response;
    if (!Client->MessageStream->Send(&Response, &Message))
    {
        WarningS(Client->GetName().c_str(), "Disconnecting client as failed to send EmptyResponse response.");
        return MessageHandleResult::Error;
    }

    return MessageHandleResult::Handled;
}

MessageHandleResult LoggingManager::Handle_RequestNotifyCreateSignResult(GameClient* Client, const Frpg2ReliableUdpMessage& Message)
{
    Frpg2RequestMessage::RequestNotifyCreateSignResult* Request = (Frpg2RequestMessage::RequestNotifyCreateSignResult*)Message.Protobuf.get();

    // Note: I don't think we really care about this log. We get most of this during the summon flow.

    Frpg2RequestMessage::EmptyResponse Response;
    if (!Client->MessageStream->Send(&Response, &Message))
    {
        WarningS(Client->GetName().c_str(), "Disconnecting client as failed to send EmptyResponse response.");
        return MessageHandleResult::Error;
    }

    return MessageHandleResult::Handled;
}

MessageHandleResult LoggingManager::Handle_RequestNotifyBreakInResult(GameClient* Client, const Frpg2ReliableUdpMessage& Message)
{
    Frpg2RequestMessage::RequestNotifyBreakInResult* Request = (Frpg2RequestMessage::RequestNotifyBreakInResult*)Message.Protobuf.get();

    // Note: I don't think we really care about this log. We get most of this during the summon flow.

    Frpg2RequestMessage::EmptyResponse Response;
    if (!Client->MessageStream->Send(&Response, &Message))
    {
        WarningS(Client->GetName().c_str(), "Disconnecting client as failed to send EmptyResponse response.");
        return MessageHandleResult::Error;
    }

    return MessageHandleResult::Handled;
}

std::string LoggingManager::GetName()
{
    return "Logging";
}

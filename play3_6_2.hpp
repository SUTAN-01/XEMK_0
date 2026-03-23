#ifndef PLAY_HPP
#define PLAY_HPP
#include "card3_6_2.hpp"
#include <algorithm>
// #include "server1_3.hpp"

using namespace std::chrono_literals;
using json = nlohmann::json;
using std::placeholders::_1;
using std::placeholders::_2;



//1212部分为旧但有出新的卡牌时，会在手牌中额外扣除重复的手牌
class play{ 

public:
    json send_move(std::vector<std::vector<Card*>> &slots_cards){
        // 发送移动接受消息
        json accept_response;
        accept_response["type"] = "move_accepted";
        // accept_response["message"] = move_desc;

        
        json card_info = json::array();
        for (auto cards : slots_cards) {
            for(auto card : cards)
            {
                if(card)
                {
                    card_info.push_back(card->toJson());
                }
            }
        }
        accept_response["cards_played"] = card_info;
        return accept_response;
    }

    void delate_cards(std::vector<Card*> &player_cards,
        std::vector<std::vector<Card*>> &player_slots_cards,
        Card* card, int &player_bones){
        // 卡牌死亡
        for (auto it = player_cards.begin(); it != player_cards.end(); ){
            if (card == *it){ 
                it = player_cards.erase(it);
                //骨头🦴+=1
                player_bones+=1;
                break;
            }
            else{
                ++it;
            }
        }
        for(auto &slot_cards : player_slots_cards){
            for(auto &&it = slot_cards.begin();it != slot_cards.end();){
                if (card == *it){
                    delete *it;
                    it = slot_cards.erase(it);
                    break;
                }
                else{
                    ++it;
                }
            }
        } 
    }


    auto safe_get_card(const std::string& player_id, int slot_idx, 
            std::unordered_map<std::string, std::vector<std::vector<Card*>>>& slots_map) -> Card* {
            // 1. 检查玩家是否存在
            auto player_it = slots_map.find(player_id);
            if (player_it == slots_map.end()) {
                std::cerr << "Player " << player_id << " not found in slots_map" << std::endl;
                return nullptr;
            }
            
            // 2. 检查槽位索引
            auto& player_slots = player_it->second;
            if (slot_idx < 0 || slot_idx >= player_slots.size()) {
                std::cerr << "Slot index " << slot_idx << " out of bounds for player " << player_id 
                        << " (size=" << player_slots.size() << ")" << std::endl;
                return nullptr;
            }
            
            // 3. 检查槽位内是否有卡牌
            auto& slot = player_slots[slot_idx];
            if (slot.empty()) {
                return nullptr;  // 空槽位
            }
            
            // 4. 返回第一张卡牌
            return slot[0];
        }

    //cur_card为当前发动攻击的牌
    void process_attribute_atk(Card* &cur_card, Card* &op_card,
        bool &cur_has_card, bool &op_has_card,int &character_HP,int cur_character,
        std::string op_player_id,std::unordered_map<std::string, std::vector<std::vector<Card*>>> op_slots_cards){
        //记录是否有卡牌
        //property当前攻击方卡牌属性
        //分被攻击和主动攻击两方处理
        //1-8考虑属性组合，最好属性固定只影响卡牌自己，并且考虑属性的先后顺序-
        int attack_flag=0;
        
        std::vector<std::string> property=cur_card->getproperty();    
        for(auto &property_i:property){ 
            if(property_i=="死神之触"){
                
                if(op_has_card){//对方卡牌的生命值
                    cur_card->setATK(op_card->getHP());
                }else{//原本的攻击数值
                    cur_card->setATK(cur_card->getATK_temp_save());
                }
            }
            else if(property_i=="空袭"){
                
                std::vector<std::string> vec = op_card->getproperty();
                auto it = std::find(vec.begin(), vec.end(), "高跳");
                if(it==vec.end()){
                    op_has_card=false;
                }
            }
            else if(property_i=="急袭"){//上场立刻攻击一次
                
                if(cur_card->card_start_stage==1){ 
                    if(op_has_card){
                        op_card->lossHP(cur_card->getATK());
                    }else{
                        if(cur_character==1){//先手方攻击回合
                            character_HP+=cur_card->getATK();
                        }else{
                            character_HP-=cur_card->getATK();
                        }
                    }
                    cur_card->card_start_stage=0;
                }
            }else if(property_i=="脆骨"){//被攻击就死
                if((op_has_card&&op_card->getATK()>0)){
                    cur_card->setHP(cur_card->getHP());
                }else{
                    cur_card->setHP(cur_card->getHP_temp_save());
                }
            }else if(property_i=="全向打击"){
                for(int i=0;i<4;i++){//统计全向伤害数
                    Card* op_card_noatk = safe_get_card(op_player_id, i, op_slots_cards);//先手方卡牌
                    if(op_card!=op_card_noatk){
                        if(op_card_noatk!=nullptr){ 
                            op_card_noatk->lossHP(cur_card->getATK());
                        }else{
                            if(cur_character==1){//先手方攻击回合
                                character_HP+=cur_card->getATK();
                            }else{
                                character_HP-=cur_card->getATK();
                            }
                        }
                    }
                }
            }
        }
    }
    
    int cur_plays(std::unordered_map<std::string, std::vector<std::vector<Card*>>> &cur_player_slots_cards,
                    std::unordered_map<std::string, std::vector<std::vector<Card*>>> &last_slots_cards,
                    std::string cur_player_id, std::string op_player_id,
                    std::unordered_map<std::string, std::vector<Card*>> &player_cards_, int &game_end,
                    int &last_player_bones,int &cur_player_bones, int &character_HP_flag) {
            // int game_end = 0;
            // 安全检查函数
            if(character_HP_flag==0){
                character_HP=0;
                character_HP_flag=1;
            }
            
            
            // 对战逻辑
            for (int i = 0; i < 4; i++) {  // 假设最多4个槽位
                // 使用安全函数获取卡牌
                Card* cur_card = safe_get_card(cur_player_id, i, cur_player_slots_cards);//后手方卡牌
                Card* op_card = safe_get_card(op_player_id, i, last_slots_cards);//先手方卡牌
                
                // 记录是否有卡牌
                bool cur_has_card = (cur_card != nullptr);
                bool op_has_card = (op_card != nullptr);
                
                // process_attribute_atk(op_card, cur_card, op_has_card, cur_has_card, character_HP, 1);
               

                // 战斗逻辑
                if (op_has_card) {  // 先手方有卡牌
                    if (!cur_has_card) {  // 后手玩家无牌
                        // 后手玩家承担对方卡牌伤害
                        character_HP += op_card->getATK();
                        
                        if (character_HP > 5) {  // 注意：应该是 >= 5 吗？
                            game_end= 1;  // 当前玩家死亡
                        }
                    } else {  // 双方都有牌
                        cur_card->lossHP(op_card->getATK());
                        if (cur_card->getHP() <= 0) {
                            //卡牌死亡
                            delate_cards(player_cards_[cur_player_id],cur_player_slots_cards[cur_player_id],cur_card,cur_player_bones);
                        }
                    }
                }
                
                // 第二段逻辑：反向攻击（当前玩家的牌攻击对方的牌）
                // 注意：这里 cur_card 和 op_card 可能已经在上面的逻辑中被修改或删除，需要重新获取
                cur_card = safe_get_card(cur_player_id, i, cur_player_slots_cards);//后手方卡牌
                op_card = safe_get_card(op_player_id, i, last_slots_cards);//先手方卡牌
                
                cur_has_card = (cur_card != nullptr);
                op_has_card = (op_card != nullptr);

                // process_attribute_atk(cur_card, op_card, cur_has_card, op_has_card,character_HP, 2);
                
                if (cur_has_card&&game_end!=1) {  // 当前玩家有牌
                    if (op_has_card) {  // 对方有牌
                        if(cur_card->getHP()>0){//后手玩家
                            op_card->lossHP(cur_card->getATK());
                        }
                        
                        if (op_card->getHP() <= 0) {
                            //卡牌死亡
                            delate_cards(player_cards_[op_player_id],last_slots_cards[op_player_id],op_card, last_player_bones);
                        }
                    }else{//先手玩家无牌且后手玩家有牌
                       
                        if(cur_card->getHP()>0){//后手玩家牌存活
                            character_HP -= cur_card->getATK();//先手玩家承担伤害
                            if (character_HP < -5) {  // 
                                game_end = -1;  
                            }
                        }

                    }
                }
            }
            
            round += 1;
        // 直接使用vector存储两个槽位向量
        std::vector<std::vector<std::vector<Card*>>> players_slots_cards{
            last_slots_cards[op_player_id],  // 对方玩家槽位
            cur_player_slots_cards[cur_player_id]  // 当前玩家槽位
        };
        return character_HP;
        // return game_end;
    }

    //12.29当前"card_placement_update"类型的信息中的"action"给出了add和clear两种，同时发送玩家对应id，
    //但是接收card_placement_update本身需要在on_massage中，同时需要对id进行判断，确保是正确的玩家进行的操作
    std::vector<std::vector<Card*>> an_slot_card(const json& data, std::unordered_map<std::string, std::vector<Card*>> &player_cards_,
        CardRandomizer &cardRandomizer,int &card_id,std::string &player_idnex,int &player_bones,
        std::vector<std::vector<Card*>> &slots_cards){
        // int out_card_num=0;
        int ind=0;
        

        // 解析 slots 数据
        if (data.contains("slots") && data["slots"].is_array()) {
            // 调试输出：打印整个 slots 数据
            // std::cout << "Slots data: " << data["slots"].dump(2) << std::endl;
            
            for (const auto& slot_data : data["slots"]) { // data["slots"]中含有四个slot_data
                std::vector<Card*> slot_cards;
                
                for (const auto& card_data : slot_data) { // slot_data中包含多个card_data
                    card_id = card_data["id"].get<int>();

                    // 在 player_cards_[player_id] 中查找并删除匹配的卡牌
                    auto& player_cards = player_cards_[player_idnex];

                    // 方法: 使用迭代器遍历和删除
                    for (auto it = player_cards.begin(); it != player_cards.end();) {
                        int player_card_id = (*it)->get_play_current_card_id();
                        if (player_card_id == card_id) {
                            auto cost_map = (*it)->getcost();
                            auto costit = cost_map.begin();
                            int state=(*it)->get_card_state();
                            if(costit!=cost_map.end()&&state!=1){//state!=1表示卡牌原本不在场上，用来防止已上场的牌反复扣除资源
                                if(costit->first=="骨头"){
                                    if(player_bones>=costit->second){
                                        //可以出牌
                                        player_bones-=costit->second;

                                        // 直接将找到的卡牌添加到 slot_cards 中
                                        if(slot_cards.empty()){
                                            slot_cards.push_back(*it);
                                            (*it)->set_card_state(1);
                                        }
                                    }
                                }
                            }else{
                                if(state!=0){  
                                    slot_cards.push_back(*it);
                                }
                            }
                            break; // 找到后跳出循环，假设卡牌ID是唯一的
                        } else {
                            // slot_cards.push_back(card_data);
                            ++it;
                        }
                    }
                }
                slots_cards[ind]=(slot_cards);
                ind++;
            }
        }

        return slots_cards;
    }

private:
    int iniflag=0;
    int round=0;
    int character_HP=0;
    int need_num=0;
    // Card* op_cur_card;
    // Card* cur_card;

    // GameServer game_server;
    // int op_character_HP=0;
    std::vector<Card*> played_cards={};

};

#endif

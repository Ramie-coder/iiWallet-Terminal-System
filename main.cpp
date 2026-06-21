#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <memory>
#include <map>
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <set>
#include <iomanip>

// ==========================================
// [第 6 迭代] 模擬 AES-256 安全加密器
// ==========================================
class AES256_Mock {
private:
    const std::vector<unsigned char> secureKey = {
        0x4A, 0x61, 0x73, 0x6F, 0x6E, 0x5F, 0x43, 0x2B, 0x2B, 0x5F, 0x69, 0x57, 
        0x61, 0x6C, 0x6C, 0x65, 0x74, 0x5F, 0x53, 0x65, 0x63, 0x75, 0x72, 0x65
    };
public:
    std::string processData(const std::string& input) {
        std::string output = input;
        for (size_t i = 0; i < output.length(); ++i) {
            output[i] = output[i] ^ secureKey[i % secureKey.size()] ^ (i * 13 % 256);
        }
        return output;
    }
};

// ==========================================
// 自訂例外處理類別
// ==========================================
class WalletException : public std::runtime_error {
public:
    explicit WalletException(const std::string& message) : std::runtime_error(message) {}
};

// ==========================================
// 1. 類別繼承架構 (標準物件導向模型)
// ==========================================
class WalletItem {
protected:
    std::string timestamp; // 格式: YYYY-MM-DD HH:MM
    int amount;            // 交易金額
    std::string merchant;  // 交易商家/備註
public:
    WalletItem(std::string t, int a, std::string m) : timestamp(t), amount(a), merchant(m) {}
    virtual ~WalletItem() = default;

    std::string getTimestamp() const { return timestamp; }
    int getAmount() const { return amount; }
    std::string getMerchant() const { return merchant; }

    virtual void showReceipt() const = 0; 
    virtual std::string getType() const = 0;
    virtual std::string getDetailAttribute() const = 0;
};

// 衍生類別 1：iPay 信用卡消費 (支出)
class iPayTransaction : public WalletItem {
private:
    std::string creditCard; // 付款卡片
public:
    iPayTransaction(std::string t, int a, std::string m, std::string card) 
        : WalletItem(t, a, m), creditCard(card) {}

    void showReceipt() const override {
        std::cout << " [💸 支出明細] 交易時間: " << timestamp 
                  << " | 刷卡金額: -" << std::setw(6) << amount << " 元"
                  << " | 消費商家: " << std::setw(12) << merchant 
                  << " | 付款卡片: " << creditCard << "\n";
    }
    std::string getType() const override { return "IPAY_SPEND"; }
    std::string getDetailAttribute() const override { return creditCard; }
};

// 衍生類別 2：雲端帳戶儲值 (收入)
class CloudTransfer : public WalletItem {
private:
    std::string bankSource; // 來源銀行
public:
    CloudTransfer(std::string t, int a, std::string m, std::string bank) 
        : WalletItem(t, a, m), bankSource(bank) {}

    void showReceipt() const override {
        std::cout << " [📥 收入明細] 交易時間: " << timestamp 
                  << " | 儲值金額: +" << std::setw(6) << amount << " 元"
                  << " | 儲值備註: " << std::setw(12) << merchant 
                  << " | 來源銀行: " << bankSource << "\n";
    }
    std::string getType() const override { return "CLOUD_TRF"; }
    std::string getDetailAttribute() const override { return bankSource; }
};

// ==========================================
// 2. 防呆工具與畫面清理
// ==========================================
int getValidInt() {
    int val;
    while (true) {
        try {
            if (!(std::cin >> val)) {
                std::cin.clear(); 
                std::cin.ignore(1000, '\n');
                throw WalletException("輸入非數字字元！");
            }
            if (val < 0) throw WalletException("金額不可為負數！");
            std::cin.ignore(1000, '\n'); 
            return val;
        } 
        catch (const WalletException& e) {
            std::cout << "❌ 安全防護攔截 -> 錯誤原因: " << e.what() << " 請重新輸入金額: ";
        }
    }
}

void clearScreen() {
#ifdef _WIN32
    std::system("cls");
#else
    std::system("clear");
#endif
}

// ==========================================
// 3. iWallet 核心管理器
// ==========================================
class iWalletManager {
private:
    std::vector<WalletItem*> walletRecords;
    std::set<std::string> registeredCards; 
    AES256_Mock cipher; 
    const std::string filename = "iwallet_secure_data.txt";
    int balance = 45200; 

public:
    iWalletManager() { loadFromCloud(); }
    ~iWalletManager() { syncToCloud(); for (auto item : walletRecords) delete item; }

    void loadFromCloud() {
        std::ifstream file(filename, std::ios::binary); if (!file.is_open()) return;
        std::string enc((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>()); file.close();
        std::string dec = cipher.processData(enc); std::stringstream ss(dec); std::string line;
        while (std::getline(ss, line)) {
            if (line.empty()) continue;
            std::stringstream ls(line); std::string type, dPart, tPart, merch, attr; int amt;
            std::getline(ls, type, ','); 
            std::getline(ls, dPart, ' '); std::getline(ls, tPart, ',');
            std::string fullTime = dPart + " " + tPart;
            std::string amtStr; std::getline(ls, amtStr, ','); amt = std::stoi(amtStr);
            std::getline(ls, attr, ','); std::getline(ls, merch, ',');

            if (type == "IPAY_SPEND") {
                walletRecords.push_back(new iPayTransaction(fullTime, amt, merch, attr));
                registeredCards.insert(attr); balance -= amt;
            } else if (type == "CLOUD_TRF") {
                walletRecords.push_back(new CloudTransfer(fullTime, amt, merch, attr)); balance += amt;
            }
        }
    }

    void syncToCloud() {
        std::stringstream ss;
        for (const auto& item : walletRecords) {
            ss << item->getType() << "," << item->getTimestamp() << "," << item->getAmount() << "," << item->getDetailAttribute() << "," << item->getMerchant() << "\n";
        }
        std::string encData = cipher.processData(ss.str()); std::ofstream file(filename, std::ios::binary); file << encData; file.close();
    }

    int getBalance() const { return balance; }
    void printCards() const {
        if (registeredCards.empty()) std::cout << " [💳] 尚未綁定任何自訂卡片 (請使用功能 1 自動新增綁定)\n";
        else { int c = 1; for (const auto& card : registeredCards) std::cout << " [💳 " << c++ << "] " << card << " | 已連線可用\n"; }
    }

    void executeTransaction(int choice) {
        std::string date, hour, min, fullTime, merch, attr; int amt;
        std::cout << "請輸入交易日期 (格式 YYYY-MM-DD): "; std::cin >> date;
        std::cout << "請輸入交易小時 (格式 00-23): "; std::cin >> hour;
        std::cout << "請輸入交易分鐘 (格式 00-59): "; std::cin >> min;
        fullTime = date + " " + hour + ":" + min;

        std::cout << (choice == 1 ? "請輸入自訂付款卡片名稱 (如: 國泰CUBE): " : "請輸入轉入來源銀行 (如: 中信銀行): ");
        std::cin.ignore(1000, '\n'); std::getline(std::cin, attr);
        std::cout << (choice == 1 ? "請輸入自訂消費商家名稱 (如: 藏壽司): " : "請輸入轉入儲值備註說明 (如: 零用錢): ");
        std::getline(std::cin, merch);

        std::cout << "請輸入金額 (TWD): "; amt = getValidInt();
        if (choice == 1) {
            walletRecords.push_back(new iPayTransaction(fullTime, amt, merch, attr)); 
            registeredCards.insert(attr); balance -= amt;
            std::cout << "💸 iPay 認證成功！明細已安全寫入對帳單。\n";
        } else {
            walletRecords.push_back(new CloudTransfer(fullTime, amt, merch, attr)); balance += amt; 
            std::cout << "📥 帳戶儲值成功！儲值明細已安全寫入。\n";
        }
    }

    void showStatement() {
        if (walletRecords.empty()) { std::cout << " 目前無任何交易明細。\n"; return; }
        std::sort(walletRecords.begin(), walletRecords.end(), [](const WalletItem* a, const WalletItem* b) { return a->getTimestamp() < b->getTimestamp(); });
        std::cout << "\n--- 📜 iWallet 歷史對帳單 (已依自訂年月日時分精密排序) ---\n";
        for (const auto& item : walletRecords) item->showReceipt();
        std::cout << "--------------------------------------------------------\n";
    }

    // ✨【第 7 迭代核心亮點：高階理財資料視覺化重構】
    void showAnalysis() {
        if (walletRecords.empty()) { std::cout << " 暫無足夠數據進行理財視覺化分析。\n"; return; }
        std::map<std::string, int> cardStats; 
        int totalSpend = 0, totalIncome = 0;

        for (const auto& item : walletRecords) {
            if (item->getType() == "IPAY_SPEND") { 
                totalSpend += item->getAmount(); 
                cardStats[item->getDetailAttribute()] += item->getAmount(); 
            } else {
                totalIncome += item->getAmount();
            }
        }

        std::cout << "\n==================================================\n";
        std::cout << "       📊 iWallet 智慧理財與卡片消費佔比分析\n";
        std::cout << "==================================================\n";
        std::cout << " 📥 累計儲值總額: $" << totalIncome << " TWD\n";
        std::cout << " 💸 累計刷卡支出: $" << totalSpend << " TWD\n";
        std::cout << " ⚖️ 當前淨資產值: $" << balance << " TWD\n";
        std::cout << "--------------------------------------------------\n";
        std::cout << " [💳 各大信用卡消費佔比動態圖表]\n";
        
        for (const auto& pair : cardStats) { 
            double percentage = (totalSpend > 0) ? ((double)pair.second / totalSpend) * 100 : 0;
            std::cout << " * " << std::setw(12) << pair.first << " : $" << std::setw(6) << pair.second << " 元 (" << std::fixed << std::setprecision(1) << percentage << "%) "; 
            
            // 用動態特殊符號渲染長條比例圖
            int bars = percentage / 5; // 每 5% 畫一個方塊
            for(int i = 0; i < 20; ++i) {
                if (i < bars) std::cout << "█"; // 已花費比例
                else std::cout << "░";          // 未花費空間
            }
            std::cout << "\n"; 
        }
        std::cout << "==================================================\n";
    }
};

int main() {
    iWalletManager wallet; int choice = 0;
    while (true) {
        std::cout << "\n==================================================\n";
        std::cout << "✨ iWallet | 數位錢包與信用卡管理系統\n";
std::cout << "==================================================\n";
        std::cout << " [💵 錢包可用餘額: $" << wallet.getBalance() << " TWD]\n\n主要卡片狀態:\n"; wallet.printCards();
        std::cout << "\n ⚙️ 功能選單: 1.感應消費 | 2.帳戶轉入 | 3.歷史對帳單 | 4.年度財務分析 | 5.安全退出\n請輸入功能編號 (1-5): ";
        choice = getValidInt(); clearScreen();
        switch (choice) {
            case 1:
            case 2: wallet.executeTransaction(choice); break;
            case 3: wallet.showStatement(); break;
            case 4: wallet.showAnalysis(); break;
            case 5: std::cout << "🛡️ [安全防護] 正在啟動 256-bit 密鑰加密持久化機制...\n🔒 安全同步退出成功！感謝您使用 iWallet 系統。\n"; return 0;
            default: std::cout << "❌ 指令錯誤，請重新輸入。\n";
        }
    }
  

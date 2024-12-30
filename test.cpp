#include "pch.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <spdlog/spdlog.h>
#include <gtest/gtest.h>

class Bank {
public:
    Bank() : cashInVault(10000) {
        logger = spdlog::stdout_color_mt("bank_logger");
    }

    void deposit(double amount) {
        std::lock_guard<std::mutex> lock(mtx);
        cashInVault += amount;
        logger->info("Deposited: {} units. Total in vault: {}", amount, cashInVault);
        cv.notify_all();
    }

    void withdraw(double amount) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this, amount] { return cashInVault >= amount; });
        cashInVault -= amount;
        logger->info("Withdrawn: {} units. Remaining in vault: {}", amount, cashInVault);
    }

    double getBalance() const {
        return cashInVault;
    }

    void monitor() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::lock_guard<std::mutex> lock(mtx);
            if (cashInVault > 20000) {
                logger->warn("Cash in vault exceeds 20000. Moving to storage.");
                // Ћогика перемещени€ денег в хранилище
            }
            if (cashInVault < 5000) {
                logger->warn("Cash in vault is less than 5000. Refilling from storage.");
                // Ћогика пополнени€ наличных из хранилища
            }
        }
    }

private:
    double cashInVault;
    std::mutex mtx;
    std::condition_variable cv;
    std::shared_ptr<spdlog::logger> logger;
};

void client(Bank& bank, double amount) {
    bank.deposit(amount);
}

void cashier(Bank& bank, double amount) {
    bank.withdraw(amount);
}

int main(int argc, char** argv) {
    auto console = spdlog::stdout_color_mt("console");
    console->info("Starting the program...");

    Bank bank;

    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();

    console->info("Program finished.");
    return result;
}

TEST(BankTest, DepositTest) {
    Bank bank;
    bank.deposit(1000);
    EXPECT_EQ(bank.getBalance(), 11000);
}

TEST(BankTest, WithdrawTest) {
    Bank bank;
    bank.withdraw(500);
    EXPECT_EQ(bank.getBalance(), 9500);
}

TEST(BankTest, OverdrawTest) {
    Bank bank;
    std::thread t1(&Bank::withdraw, &bank, 5000);
    std::thread t2(&Bank::withdraw, &bank, 6000);
    t1.join();
    t2.join();
    EXPECT_GE(bank.getBalance(), 0);
}

TEST(BankTest, MultiThreadDepositTest) {
    Bank bank;
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back(&Bank::deposit, &bank, 1000);
    }
    for (auto& th : threads) {
        th.join();
    }
    EXPECT_EQ(bank.getBalance(), 20000);
}

TEST(BankTest, MultiThreadWithdrawTest) {
    Bank bank;
    std::vector<std::thread> threads;
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back(&Bank::withdraw, &bank, 1000);
    }
    for (auto& th : threads) {
        th.join();
    }
    EXPECT_EQ(bank.getBalance(), 5000);
}

TEST(BankTest, MixedOperationsTest) {
    Bank bank;
    std::thread t1(&Bank::deposit, &bank, 5000);
    std::thread t2(&Bank::withdraw, &bank, 3000);
    t1.join();
    t2.join();
    EXPECT_EQ(bank.getBalance(), 12000);
}

TEST(BankTest, MonitorWarnTest) {
    Bank bank;
    bank.deposit(15000); // ѕриведет к превышению лимита наличных
    std::this_thread::sleep_for(std::chrono::seconds(2));
    // ѕроверка логов на наличие предупреждени€
}

TEST(BankTest, MonitorRefillTest) {
    Bank bank;
    bank.withdraw(6000); // ѕриведет к понижению наличных
    std::this_thread::sleep_for(std::chrono::seconds(2));
    // ѕроверка логов на наличие предупреждени€
}

TEST(BankTest, ConcurrentDepositWithdrawTest) {
    Bank bank;
    std::thread t1(&Bank::deposit, &bank, 2000);
    std::thread t2(&Bank::withdraw, &bank, 1500);
    t1.join();
    t2.join();
    EXPECT_EQ(bank.getBalance(), 10500);
}

TEST(BankTest, EdgeCaseNegativeBalanceTest) {
    Bank bank;
    std::thread t1(&Bank::withdraw, &bank, 12000);
    std::thread t2(&Bank::withdraw, &bank, 5000);
    t1.join();
    t2.join();
    EXPECT_EQ(bank.getBalance(), 0);
}

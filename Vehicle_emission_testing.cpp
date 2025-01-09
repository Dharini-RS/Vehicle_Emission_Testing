//Implementation of Vehicle emission testing
#include <iostream>
#include <unordered_map>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <stdexcept>

// Emission Strategy Interface
class EmissionStrategy {
public:
    virtual double calculateEmission(double parameter) const = 0;
    virtual ~EmissionStrategy() = default;
};

// Concrete Strategy: Gas Emission
class GasEmissionStrategy : public EmissionStrategy {
public:
    double calculateEmission(double engineSize) const override {
        return engineSize * 0.1; // Dummy formula for emission level
    }
};

// Concrete Strategy: Electric Emission
class ElectricEmissionStrategy : public EmissionStrategy {
public:
    double calculateEmission(double batteryCapacity) const override {
        return 0; // EVs have zero emissions
    }
};

// Base Vehicle Class
class Vehicle {
protected:
    std::string type;
    int age;
    std::string emissionStandard;
    std::shared_ptr<EmissionStrategy> emissionStrategy;

public:
    Vehicle(std::string t, int a, std::string e, std::shared_ptr<EmissionStrategy> strategy)
        : type(t), age(a), emissionStandard(e), emissionStrategy(strategy) {}
    virtual ~Vehicle() = default;

    virtual void displayDetails() const {
        std::cout << "Vehicle Type: " << type << "\nAge: " << age 
                  << "\nEmission Standard: " << emissionStandard << std::endl;
    }

    virtual double getEmissionLevel() const = 0; // Pure virtual function
};

// Gas Vehicle Class
class GasVehicle : public Vehicle {
private:
    double engineSize;

public:
    GasVehicle(int a, std::string e, double size, std::shared_ptr<EmissionStrategy> strategy) 
        : Vehicle("Gas", a, e, strategy), engineSize(size) {}

    double getEmissionLevel() const override {
        return emissionStrategy->calculateEmission(engineSize);
    }

    void displayDetails() const override {
        Vehicle::displayDetails();
        std::cout << "Engine Size: " << engineSize << " cc" << std::endl;
    }
};

// Electric Vehicle Class
class ElectricVehicle : public Vehicle {
private:
    double batteryCapacity;

public:
    ElectricVehicle(int a, std::string e, double capacity, std::shared_ptr<EmissionStrategy> strategy) 
        : Vehicle("Electric", a, e, strategy), batteryCapacity(capacity) {}

    double getEmissionLevel() const override {
        return emissionStrategy->calculateEmission(batteryCapacity);
    }

    void displayDetails() const override {
        Vehicle::displayDetails();
        std::cout << "Battery Capacity: " << batteryCapacity << " kWh" << std::endl;
    }
};

// State Interface for Emission Test
class EmissionTestState {
public:
    virtual void handleTest(std::shared_ptr<class EmissionTest> test, std::shared_ptr<Vehicle> vehicle, double legalLimit) = 0;
    virtual ~EmissionTestState() = default;
};

// Forward declaration of EmissionTest class
class EmissionTest;

// Concrete State: Pending
class PendingState : public EmissionTestState {
public:
    void handleTest(std::shared_ptr<EmissionTest> test, std::shared_ptr<Vehicle> vehicle, double legalLimit) override;
};

// Concrete State: InProgress
class InProgressState : public EmissionTestState {
public:
    void handleTest(std::shared_ptr<EmissionTest> test, std::shared_ptr<Vehicle> vehicle, double legalLimit) override;
};

// Concrete State: Completed
class CompletedState : public EmissionTestState {
public:
    void handleTest(std::shared_ptr<EmissionTest> test, std::shared_ptr<Vehicle> vehicle, double legalLimit) override;
};

// Emission Test Class
class EmissionTest : public std::enable_shared_from_this<EmissionTest> {
private:
    std::string vehicleID;
    std::shared_ptr<EmissionTestState> state;
    bool complianceStatus;

public:
    EmissionTest(const std::string &id, std::shared_ptr<EmissionTestState> initialState)
        : vehicleID(id), state(initialState), complianceStatus(false) {}

    void setState(std::shared_ptr<EmissionTestState> newState) {
        state = newState;
    }

    void performTest(std::shared_ptr<Vehicle> vehicle, double legalLimit) {
        state->handleTest(shared_from_this(), vehicle, legalLimit);
    }

    void setComplianceStatus(bool status) {
        complianceStatus = status;
    }

    bool getComplianceStatus() const {
        return complianceStatus;
    }

    std::string getVehicleID() const {
        return vehicleID;
    }
};

// Implementations of State Handlers
void PendingState::handleTest(std::shared_ptr<EmissionTest> test, std::shared_ptr<Vehicle> vehicle, double legalLimit) {
    std::cout << "Test for " << test->getVehicleID() << " is now in progress.\n";
    test->setState(std::make_shared<InProgressState>());
    test->performTest(vehicle, legalLimit);
}

void InProgressState::handleTest(std::shared_ptr<EmissionTest> test, std::shared_ptr<Vehicle> vehicle, double legalLimit) {
    double emissionLevel = vehicle->getEmissionLevel();
    if (emissionLevel < 0) {
        throw std::invalid_argument("Invalid emission level.");
    }

    bool complianceStatus = (emissionLevel <= legalLimit);
    test->setComplianceStatus(complianceStatus);
    test->setState(std::make_shared<CompletedState>());

    std::cout << "Vehicle ID: " << test->getVehicleID() 
              << " | Emission Level: " << emissionLevel 
              << " | Compliance: " << (complianceStatus ? "Pass" : "Fail") << std::endl;
}

void CompletedState::handleTest(std::shared_ptr<EmissionTest> test, std::shared_ptr<Vehicle> vehicle, double legalLimit) {
    std::cout << "Test for " << test->getVehicleID() << " is already completed.\n";
}

// Manage Test Results
std::unordered_map<std::string, bool> testResults;
std::mutex resultMutex;

void runTest(std::shared_ptr<Vehicle> vehicle, const std::string &id, double legalLimit) {
    try {
        auto test = std::make_shared<EmissionTest>(id, std::make_shared<PendingState>());
        test->performTest(vehicle, legalLimit);

        // Store results safely
        std::lock_guard<std::mutex> lock(resultMutex);
        testResults[id] = test->getComplianceStatus();
    } catch (const std::invalid_argument &e) {
        std::cerr << "Invalid argument for Vehicle ID " << id << ": " << e.what() << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Error for Vehicle ID " << id << ": " << e.what() << std::endl;
    }
}

// Main Function
int main() {
    // Create Emission Strategies
    std::shared_ptr<EmissionStrategy> gasStrategy = std::make_shared<GasEmissionStrategy>();
    std::shared_ptr<EmissionStrategy> electricStrategy = std::make_shared<ElectricEmissionStrategy>();

    // Create Vehicle objects
    std::vector<std::shared_ptr<Vehicle>> vehicles = {
        std::make_shared<GasVehicle>(5, "BS6", 2000.0, gasStrategy),
        std::make_shared<ElectricVehicle>(2, "EV", 50.0, electricStrategy),
        std::make_shared<GasVehicle>(10, "BS4", 1500.0, gasStrategy)
    };

    // Legal emission limit
    double legalLimit = 180.0;

    // Run emission tests concurrently
    std::vector<std::thread> testThreads;
    int vehicleID = 1;
    for (auto &vehicle : vehicles) {
        testThreads.emplace_back(runTest, vehicle, "Vehicle_" + std::to_string(vehicleID++), legalLimit);
    }

    // Wait for all threads to complete
    for (auto &thread : testThreads) {
        thread.join();
    }

    // Menu for user inputs
    while (true) {
        std::cout << "\nMenu:\n";
        std::cout << "1. View Test Results\n";
        std::cout << "2. Check Vehicle Details\n";
        std::cout << "3. Exit\n";
        std::cout << "Enter your choice: ";
        
        int choice;
        std::cin >> choice;

        if (choice == 1) {
            std::cout << "\nTest Results:\n";
            for (const auto &result : testResults) {
                std::cout << result.first << ": " << (result.second ? "Pass" : "Fail") << std::endl;
            }
        } else if (choice == 2) {
            std::string inputID;
            std::cout << "\nEnter Vehicle ID to see details (e.g., Vehicle_1): ";
            std::cin >> inputID;

            int index = std::stoi(inputID.substr(8)) - 1;
            if (index >= 0 && index < vehicles.size()) {
                vehicles[index]->displayDetails();
            } else {
                std::cout << "Invalid Vehicle ID." << std::endl;
            }
        } else if (choice == 3) {
            break;
        } else {
            std::cout << "Invalid choice. Please try again." << std::endl;
        }
    }

    return 0;
}

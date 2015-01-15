#include <stdio.h>
#include <iostream>
#include <sstream>
#include <queue>
#include <set>
#include <map>
#include "action_authoring/ConstraintMacro.h"
#include "action_authoring/DatabaseManager.h"
#include "action_authoring/OrderedMap.h"
#include <lcm/lcm-cpp.hpp>
#include <lcmtypes/drc_lcmtypes.hpp>
#include <boost/thread.hpp>

using namespace action_authoring;
using namespace std;
using namespace boost;
using namespace affordance;
using namespace Eigen;

#define LCM_MESSAGE_CHANNEL "test_database_manager"

void tabprintf(std::string string, int num_tabs)
{
    for (int i = 0; i < num_tabs; i++)
    {
        printf("\t");
    }

    printf("%s\n", string.c_str());
}

void printAtomicConstraint(AtomicConstraintPtr atomicConstraint, int num_tabs)
{
    std::string relationStateString;

    stringstream ss;
    ss << atomicConstraint->getRelationState()->getRelationType();
    relationStateString = ss.str();

    tabprintf("Atomic Constraint - " + relationStateString, num_tabs);
    tabprintf(atomicConstraint->getManipulator()->getName(), num_tabs);
    tabprintf(atomicConstraint->getAffordance()->getName(), num_tabs);
}

void printConstraintMacro(ConstraintMacroPtr constraint, int num_tabs = 0)
{
    std::string constraintString;

    switch (constraint->getConstraintMacroType())
    {
    case ConstraintMacro::ATOMIC:
        constraintString = "ATOMIC ";
        break;
    case ConstraintMacro::SEQUENTIAL:
        constraintString = "SEQUENTIAL ";
        break;
    default:
        constraintString = "UNKNOWN ";
        break;
    }

    tabprintf(constraintString += constraint->getName(), num_tabs);

    if (constraint->getConstraintMacroType() == ConstraintMacro::ATOMIC)
    {
        printAtomicConstraint(constraint->getAtomicConstraint(), num_tabs + 1);
    }
    else
    {
        vector<ConstraintMacroPtr> constraints;
        constraint->getConstraintMacros(constraints);

        for (int i = 0; i < (int)constraints.size(); i++)
        {
            printConstraintMacro(constraints[i], num_tabs + 1);
        }
    }
}

void runLcmHandleLoop(const shared_ptr<lcm::LCM> theLcm)
{
    //lcm loop
    cout << "\nstarting lcm loop" << endl;

    while (0 == theLcm->handle())
    {
        ;
    }
}

int main()
{

    ManipulatorStateConstPtr rhand(new ManipulatorState("Right Hand", GlobalUID(rand(), rand())));
    ManipulatorStateConstPtr lhand(new ManipulatorState("Left Hand", GlobalUID(rand(), rand())));
    ManipulatorStateConstPtr rfoot(new ManipulatorState("Right Foot", GlobalUID(rand(), rand())));
    ManipulatorStateConstPtr lfoot(new ManipulatorState("Left Foot", GlobalUID(rand(), rand())));

    AffConstPtr wheel(new AffordanceState(0, 0, KDL::Frame(KDL::Vector(0, 0, 0)), Eigen::Vector3f(1, 0, 0)));
    AffConstPtr gas(new AffordanceState(0, 1, KDL::Frame(KDL::Vector(1, 0, 0)), Eigen::Vector3f(1, 0, 0)));
    AffConstPtr brake(new AffordanceState(0, 2, KDL::Frame(KDL::Vector(0, 1, 0)), Eigen::Vector3f(0, 1, 0)));

    RelationStatePtr relstate(new RelationState(RelationState::POINT_CONTACT));

    AtomicConstraintPtr rfoot_gas_relation(new ManipulationConstraint(gas, rhand, relstate));
    AtomicConstraintPtr lfoot_brake_relation(new ManipulationConstraint(brake, lfoot, relstate));
    AtomicConstraintPtr rhand_wheel_relation(new ManipulationConstraint(wheel, rhand, relstate));
    AtomicConstraintPtr lhand_wheel_relation(new ManipulationConstraint(wheel, lhand, relstate));

    ConstraintMacroPtr rfoot_gas(new ConstraintMacro("Right Foot to Gas Pedal", rfoot_gas_relation));
    ConstraintMacroPtr lfoot_brake(new ConstraintMacro("Left Foot to Brake Pedal", lfoot_brake_relation));
    ConstraintMacroPtr rhand_wheel(new ConstraintMacro("Right Hand To Wheel", rhand_wheel_relation));
    ConstraintMacroPtr lhand_wheel(new ConstraintMacro("Left Hand To Wheel", lhand_wheel_relation));

    ConstraintMacroPtr hands(new ConstraintMacro("Hands", ConstraintMacro::SEQUENTIAL));
    hands->appendConstraintMacro(rhand_wheel);
    hands->appendConstraintMacro(lhand_wheel);

    ConstraintMacroPtr feet(new ConstraintMacro("Feet", ConstraintMacro::SEQUENTIAL));
    feet->appendConstraintMacro(rfoot_gas);
    feet->appendConstraintMacro(lfoot_brake);

    ConstraintMacroPtr ingress(new ConstraintMacro("Ingress", ConstraintMacro::SEQUENTIAL));
    ingress->appendConstraintMacro(hands);
    ingress->appendConstraintMacro(feet);

    vector<AffConstPtr> affordanceList;
    affordanceList.push_back(wheel);
    affordanceList.push_back(gas);
    affordanceList.push_back(brake);

    std::vector<ConstraintMacroPtr> constraintList;
    constraintList.push_back(ingress);

    //print the top level constraint
    printf("\nThis is a constraint created in the function\n");
    printConstraintMacro(ingress);


    printf("Storing.\n");
    //to store state, use the db->store method and pass in all the data at once
    DatabaseManager::store("constraints.xml", affordanceList, constraintList);

    printf("\nDone storing.\n");

    printf("\nRetrieving from file.\n\n");

    vector<AffConstPtr> revivedAffordances;
    vector<ConstraintMacroPtr> revivedConstraintMacros;
    DatabaseManager::retrieve("constraints.xml", revivedAffordances, revivedConstraintMacros);

    printf("\nDone retrieving.\n");


    //print the same constraint as created before, but this time using the data from the file

    printf("\nThis is the same constraint, written to and then reconstructed from a file:\n");

    for (int i = 0; i < (int)revivedConstraintMacros.size(); i++)
    {
        if (revivedConstraintMacros[i]->getName() == "Ingress")
        {
            printConstraintMacro(revivedConstraintMacros[i]);
        }
    }

    OrderedMap<string, AffConstPtr> StorageUIDToAffordance;
    StorageUIDToAffordance["id1"] = wheel;
    StorageUIDToAffordance["id3"] = gas;
    StorageUIDToAffordance["id2"] = brake;
    StorageUIDToAffordance["id3"] = brake;


    vector<string> ids = StorageUIDToAffordance.getOrderedKeys();

    for (int i = 0; i < (int)ids.size(); i++)
    {
        printf("%s\n", ids[i].c_str());
    }

    vector<AffConstPtr> affordances = StorageUIDToAffordance.getOrderedValues();

    for (int i = 0; i < (int)affordances.size(); i++)
    {
        printf("%s\n", affordances[i]->getName().c_str());
    }


    printf("Done Testing Store and Retrieve.\n");
    printf("Now Testing LCM\n");

    shared_ptr<lcm::LCM> theLcm(new lcm::LCM());

    if (!theLcm->good())
    {
        cerr << "Cannot create lcm object" << endl;
        return -1;
    }

    printf("Created the LCM object\n");
    boost::thread testThread = boost::thread(runLcmHandleLoop, theLcm);

    printf("Created the thread \n");

    //flatten the ingress constraint
    std::vector<drc::contact_goal_t> contact_goals = ingress->toLCM();

    printf("toLCM finished\n");

    drc::action_sequence_t actionSequence;
    printf("Created the action sequence\n");
    actionSequence.num_contact_goals = (int) contact_goals.size();
    printf("Added the number of contact goals\n");
    actionSequence.contact_goals = contact_goals;
    printf("Added the contact goals list\n");
    actionSequence.robot_name = "foo bar robot";
    actionSequence.utime = 0;

    theLcm->publish(LCM_MESSAGE_CHANNEL, &actionSequence);
    printf("Published the message.\n");

    return(0);
}
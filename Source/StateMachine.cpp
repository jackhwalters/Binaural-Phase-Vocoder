/*
  ==============================================================================

        DAFX BINAURAL PHASE VOCODER
        v1.0
        Jack Walters

  ==============================================================================
*/

#include "StateMachine.h"

ImpulseSelectionStateMachine::ImpulseSelectionStateMachine()
{
    LL = 0;
    UL = 0;
    LR = 0;
    UR = 0;
}

/*
  * @brief Output the list number of the 4 nearest Head Related Impulse Responses (HRIRs) to the user's selected azimuth and elevation angle
  * @param User-specified azimuth
  * @param User-specified elevation
*/
void ImpulseSelectionStateMachine::stateMachine(float azimuth, int elevation)
{
    // As the azimth reading wraps around 0 radians (i.e. goes from 0 to π, and then from -π back to 0), but we need readings between 0-360° we need to unwrap the reading if it is between -π and 0, and then convert to degrees regardless of azimuth value
    if (azimuth < M_PI && azimuth >= 0)
    {
        azimuth = rad2deg(azimuth);
    }
    else if (azimuth < 0 && azimuth > -M_PI)
    {
        azimuth = rad2deg((M_PI - abs(azimuth)) + M_PI);
    }
    
    // State machine
    if (azimuth >= 0 && azimuth < 30 && elevation >= -90 && elevation < -60)
    {
        LL = 2;
        UL = 1;
        LR = 2;
        UR = 8;
    }
    else if (azimuth >= 0 && azimuth < 30 && elevation >= -60 && elevation < -30)
    {
        LL = 1;
        UL = 0;
        LR = 8;
        UR = 7;
    }
    else if (azimuth >= 0 && azimuth < 30 && elevation >= -30 && elevation < 0)
    {
        LL = 0;
        UL = 3;
        LR = 7;
        UR = 9;
    }
    else if (azimuth >= 0 && azimuth < 30 && elevation >= 0 && elevation < 30)
    {
        LL = 59;
        UL = 60;
        LR = 9;
        UR = 10;
    }
    else if (azimuth >= 0 && azimuth < 30 && elevation >= 30 && elevation < 60)
    {
        LL = 4;
        UL = 5;
        LR = 10;
        UR = 11;
    }
    else if (azimuth >= 0 && azimuth < 30 && elevation >= 60 && elevation <= 90)
    {
        LL = 5;
        UL = 6;
        LR = 11;
        UR = 6;
    }
    //
    else if (azimuth >= 30 && azimuth < 60 && elevation >= -90 && elevation < -60)
    {
        LL = 2;
        UL = 8;
        LR = 2;
        UR = 13;
    }
    else if (azimuth >= 30 && azimuth < 60 && elevation >= -60 && elevation < -30)
    {
        LL = 8;
        UL = 7;
        LR = 13;
        UR = 12;
    }
    else if (azimuth >= 30 && azimuth < 60 && elevation >= -30 && elevation < 0)
    {
        LL = 7;
        UL = 9;
        LR = 12;
        UR = 14;
    }
    else if (azimuth >= 30 && azimuth < 60 && elevation >= 0 && elevation < 30)
    {
        LL = 9;
        UL = 10;
        LR = 14;
        UR = 15;
    }
    else if (azimuth >= 30 && azimuth < 60 && elevation >= 30 && elevation < 60)
    {
        LL = 10;
        UL = 11;
        LR = 15;
        UR = 16;
    }
    else if (azimuth >= 30 && azimuth < 60 && elevation >= 60 && elevation <= 90)
    {
        LL = 11;
        UL = 6;
        LR = 16;
        UR = 6;
    }
    
    else if (azimuth >= 60 && azimuth <= 90 && elevation >= -90 && elevation < -60)
    {
        LL = 2;
        UL = 13;
        LR = 2;
        UR = 18;
    }
    else if (azimuth >= 60 && azimuth <= 90 && elevation >= -60 && elevation < -30)
    {
        LL = 13;
        UL = 12;
        LR = 18;
        UR = 17;
    }
    else if (azimuth >= 60 && azimuth <= 90 && elevation >= -30 && elevation < 0)
    {
        LL = 12;
        UL = 14;
        LR = 17;
        UR = 19;
    }
    else if (azimuth >= 60 && azimuth <= 90 && elevation >= 0 && elevation < 30)
    {
        LL = 14;
        UL = 15;
        LR = 19;
        UR = 20;
    }
    else if (azimuth >= 60 && azimuth <= 90 && elevation >= 30 && elevation < 60)
    {
        LL = 15;
        UL = 16;
        LR = 20;
        UR = 21;
    }
    else if (azimuth >= 60 && azimuth <= 90 && elevation >= 60 && elevation <= 90)
    {
        LL = 16;
        UL = 6;
        LR = 21;
        UR = 6;
    }

    else if (azimuth >= 90 && azimuth < 120 && elevation >= -90 && elevation < -60)
    {
        LL = 2;
        UL = 18;
        LR = 2;
        UR = 28;
    }
    else if (azimuth >= 90 && azimuth < 120 && elevation >= -60 && elevation < -30)
    {
        LL = 18;
        UL = 17;
        LR = 23;
        UR = 22;
    }
    else if (azimuth >= 90 && azimuth < 120 && elevation >= -30 && elevation < 0)
    {
        LL = 17;
        UL = 19;
        LR = 22;
        UR = 24;
    }
    else if (azimuth >= 90 && azimuth < 120 && elevation >= 0 && elevation < 30)
    {
        LL = 19;
        UL = 20;
        LR = 24;
        UR = 25;
    }
    else if (azimuth >= 90 && azimuth < 120 && elevation >= 30 && elevation < 60)
    {
        LL = 20;
        UL = 21;
        LR = 25;
        UR = 26;
    }
    else if (azimuth >= 90 && azimuth < 120 && elevation >= 60 && elevation <= 90)
    {
        LL = 21;
        UL = 6;
        LR = 26;
        UR = 6;
    }

    else if (azimuth >= 120 && azimuth < 150 && elevation >= -90 && elevation < -60)
    {
        LL = 2;
        UL = 23;
        LR = 2;
        UR = 28;
    }
    else if (azimuth >= 120 && azimuth < 150 && elevation >= -60 && elevation < -30)
    {
        LL = 23;
        UL = 22;
        LR = 28;
        UR = 27;
    }
    else if (azimuth >= 120 && azimuth < 150 && elevation >= -30 && elevation < 0)
    {
        LL = 22;
        UL = 24;
        LR = 27;
        UR = 29;
    }
    else if (azimuth >= 120 && azimuth < 150 && elevation >= 0 && elevation < 30)
    {
        LL = 24;
        UL = 25;
        LR = 29;
        UR = 30;
    }
    else if (azimuth >= 120 && azimuth < 150 && elevation >= 30 && elevation < 60)
    {
        LL = 25;
        UL = 26;
        LR = 30;
        UR = 31;
    }
    else if (azimuth >= 120 && azimuth < 150 && elevation >= 60 && elevation <= 90)
    {
        LL = 26;
        UL = 6;
        LR = 31;
        UR = 6;
    }

    else if (azimuth >= 150 && azimuth < 180 && elevation >= -90 && elevation < -60)
    {
        LL = 2;
        UL = 33;
        LR = 2;
        UR = 38;
    }
    else if (azimuth >= 150 && azimuth < 180 && elevation >= -60 && elevation < -30)
    {
        LL = 28;
        UL = 27;
        LR = 33;
        UR = 32;
    }
    else if (azimuth >= 150 && azimuth < 180 && elevation >= -30 && elevation < 0)
    {
        LL = 30;
        UL = 29;
        LR = 32;
        UR = 34;
    }
    else if (azimuth >= 150 && azimuth < 180 && elevation >= 0 && elevation < 30)
    {
        LL = 29;
        UL = 30;
        LR = 34;
        UR = 45;
    }
    else if (azimuth >= 150 && azimuth < 180 && elevation >= 30 && elevation < 60)
    {
        LL = 30;
        UL = 31;
        LR = 35;
        UR = 36;
    }
    else if (azimuth >= 150 && azimuth < 180 && elevation >= 60 && elevation <= 90)
    {
        LL = 26;
        UL = 6;
        LR = 36;
        UR = 6;
    }

    else if (azimuth >= 180 && azimuth < 210 && elevation >= -90 && elevation < -60)
    {
        LL = 2;
        UL = 38;
        LR = 2;
        UR = 38;
    }
    else if (azimuth >= 180 && azimuth < 210 && elevation >= -60 && elevation < -30)
    {
        LL = 33;
        UL = 32;
        LR = 38;
        UR = 37;
    }
    else if (azimuth >= 180 && azimuth < 210 && elevation >= -30 && elevation < 0)
    {
        LL = 32;
        UL = 34;
        LR = 37;
        UR = 39;
    }
    else if (azimuth >= 180 && azimuth < 210 && elevation >= 0 && elevation < 30)
    {
        LL = 34;
        UL = 35;
        LR = 39;
        UR = 40;
    }
    else if (azimuth >= 180 && azimuth < 210 && elevation >= 30 && elevation < 60)
    {
        LL = 35;
        UL = 36;
        LR = 40;
        UR = 41;
    }
    else if (azimuth >= 180 && azimuth < 210 && elevation >= 60 && elevation <= 90)
    {
        LL = 36;
        UL = 6;
        LR = 41;
        UR = 6;
    }

    else if (azimuth >= 210 && azimuth < 240 && elevation >= -90 && elevation < -60)
    {
        LL = 2;
        UL = 38;
        LR = 2;
        UR = 43;
    }
    else if (azimuth >= 210 && azimuth < 240 && elevation >= -60 && elevation < -30)
    {
        LL = 38;
        UL = 37;
        LR = 43;
        UR = 42;
    }
    else if (azimuth >= 210 && azimuth < 240 && elevation >= -30 && elevation < 0)
    {
        LL = 37;
        UL = 39;
        LR = 42;
        UR = 44;
    }
    else if (azimuth >= 210 && azimuth < 240 && elevation >= 0 && elevation < 30)
    {
        LL = 39;
        UL = 40;
        LR = 44;
        UR = 45;
    }
    else if (azimuth >= 210 && azimuth < 240 && elevation >= 30 && elevation < 60)
    {
        LL = 40;
        UL = 41;
        LR = 45;
        UR = 46;
    }
    else if (azimuth >= 210 && azimuth < 240 && elevation >= 60 && elevation <= 90)
    {
        LL = 41;
        UL = 6;
        LR = 46;
        UR = 6;
    }

    else if (azimuth >= 240 && azimuth < 270 && elevation >= -90 && elevation < -60)
    {
        LL = 2;
        UL = 43;
        LR = 2;
        UR = 43;
    }
    else if (azimuth >= 240 && azimuth < 270 && elevation >= -60 && elevation < -30)
    {
        LL = 43;
        UL = 42;
        LR = 48;
        UR = 47;
    }
    else if (azimuth >= 240 && azimuth < 270 && elevation >= -30 && elevation < 0)
    {
        LL = 42;
        UL = 44;
        LR = 47;
        UR = 49;
    }
    else if (azimuth >= 240 && azimuth < 270 && elevation >= 0 && elevation < 30)
    {
        LL = 44;
        UL = 40;
        LR = 49;
        UR = 50;
    }
    else if (azimuth >= 240 && azimuth < 270 && elevation >= 30 && elevation < 60)
    {
        LL = 45;
        UL = 46;
        LR = 50;
        UR = 51;
    }
    else if (azimuth >= 240 && azimuth < 270 && elevation >= 60 && elevation <= 90)
    {
        LL = 46;
        UL = 6;
        LR = 51;
        UR = 6;
    }

    else if (azimuth >= 270 && azimuth < 300 && elevation >= -90 && elevation < -60)
    {
        LL = 2;
        UL = 48;
        LR = 2;
        UR = 53;
    }
    else if (azimuth >= 270 && azimuth < 300 && elevation >= -60 && elevation < -30)
    {
        LL = 48;
        UL = 47;
        LR = 53;
        UR = 52;
    }
    else if (azimuth >= 270 && azimuth < 300 && elevation >= -30 && elevation < 0)
    {
        LL = 47;
        UL = 49;
        LR = 52;
        UR = 54;
    }
    else if (azimuth >= 270 && azimuth < 300 && elevation >= 0 && elevation < 30)
    {
        LL = 49;
        UL = 50;
        LR = 54;
        UR = 55;
    }
    else if (azimuth >= 270 && azimuth < 300 && elevation >= 30 && elevation < 60)
    {
        LL = 50;
        UL = 51;
        LR = 55;
        UR = 56;
    }
    else if (azimuth >= 270 && azimuth < 300 && elevation >= 60 && elevation <= 90)
    {
        LL = 51;
        UL = 6;
        LR = 51;
        UR = 6;
    }

    else if (azimuth >= 300 && azimuth < 330 && elevation >= -90 && elevation < -60)
    {
        LL = 2;
        UL = 53;
        LR = 2;
        UR = 58;
    }
    else if (azimuth >= 300 && azimuth < 330 && elevation >= -60 && elevation < -30)
    {
        LL = 53;
        UL = 52;
        LR = 58;
        UR = 57;
    }
    else if (azimuth >= 300 && azimuth < 330 && elevation >= -30 && elevation < 0)
    {
        LL = 52;
        UL = 54;
        LR = 57;
        UR = 59;
    }
    else if (azimuth >= 300 && azimuth < 330 && elevation >= 0 && elevation < 30)
    {
        LL = 54;
        UL = 55;
        LR = 59;
        UR = 60;
    }
    else if (azimuth >= 300 && azimuth < 330 && elevation >= 30 && elevation < 60)
    {
        LL = 55;
        UL = 56;
        LR = 60;
        UR = 61;
    }
    else if (azimuth >= 300 && azimuth < 330 && elevation >= 60 && elevation <= 90)
    {
        LL = 56;
        UL = 6;
        LR = 61;
        UR = 6;
    }

    else if (azimuth >= 330 && azimuth < 360 && elevation >= -90 && elevation < -60)
    {
        LL = 2;
        UL = 58;
        LR = 2;
        UR = 1;
    }
    else if (azimuth >= 330 && azimuth < 360 && elevation >= -60 && elevation < -30)
    {
        LL = 58;
        UL = 57;
        LR = 1;
        UR = 0;
    }
    else if (azimuth >= 330 && azimuth < 360 && elevation >= -30 && elevation < 0)
    {
        LL = 57;
        UL = 59;
        LR = 0;
        UR = 3;
    }
    else if (azimuth >= 330 && azimuth < 360 && elevation >= 0 && elevation < 30)
    {
        LL = 59;
        UL = 60;
        LR = 3;
        UR = 4;
    }
    else if (azimuth >= 330 && azimuth < 360 && elevation >= 30 && elevation < 60)
    {
        LL = 60;
        UL = 61;
        LR = 4;
        UR = 5;
    }
    else if (azimuth >= 330 && azimuth < 360 && elevation >= 60 && elevation <= 90)
    {
        LL = 61;
        UL = 6;
        LR = 5;
        UR = 6;
    }
}

ImpulseSelectionStateMachine::~ImpulseSelectionStateMachine()
{
    
}

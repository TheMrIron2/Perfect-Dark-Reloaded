//
// weapon_defs.qc - Weapon Definitions (ammo counts, mag count, dmg, etc.)
//

float(float wep) getWeaponMag = {
    switch(wep) {
        case W_FALCON2:
            return 8;
        default: 
            return 0;
    }

    return 0;
}

float(float wep) getWeaponReserve = {
    switch(wep) {
        case W_FALCON2:
            return 72;
        default: 
            return 0;
    }

    return 0;
}

string(float view, float wep) getWeaponModel = {
    if (view) {
        switch(wep) {
            case W_FALCON2:
                return "models/weapons/falcon2/v_falcon2.mdl";
            default:
                return "";
        }
    } else {
        return "";
    }
}
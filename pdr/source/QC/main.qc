void() precaches;

void() main = {

}

void() worldspawn = {
    precaches();
}

void() SetNewParms = {

}

void() SetChangeParms = {

}

void() StartFrame = {

}

void() precaches= {
    precache_model("progs/player.mdl");
    precache_model("models/weapons/falcon2/v_falcon2.mdl");
}
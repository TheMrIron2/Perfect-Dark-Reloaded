void() ClientKill = {

}

void() ClientConnect = {

}

void() ClientDisconnect = {

}

void() PlayerPreThink = {

}

void() PlayerPostThink = {

}

void() PutClientInServer = {
    local entity spawnspot;
    spawnspot = find(world, classname, "info_player_start");

    self.classname = "player";
    self.health = 100;
    self.takedamage = DAMAGE_AIM;
    self.solid = SOLID_SLIDEBOX;
    self.movetype = MOVETYPE_WALK;
    self.flags = FL_CLIENT;

    self.origin = spawnspot.origin + '0 0 1';
    self.angles = spawnspot.angles;
    self.fixangle = TRUE;

    setmodel(self, "progs/player.mdl");
    setsize(self, VEC_HULL_MIN, VEC_HULL_MAX);

    self.view_ofs = '0 0 22';
    self.velocity = '0 0 0';
}
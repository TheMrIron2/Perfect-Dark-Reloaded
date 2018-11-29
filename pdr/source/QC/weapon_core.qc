//
// weapon_core.qc - Firing, reloading, etc.
//

void() W_Fire = {
    self.currentmag--;
    centerprint(self, strcat(ftos(self.currentmag), "/", ftos(self.currentammo)));
}


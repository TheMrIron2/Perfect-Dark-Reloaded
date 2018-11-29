//
// impulses.qc - Impulse parsing/handling
//
void() W_Fire;

void() checkImpulses = {
    switch(self.impulse) {
        case 10:
            W_Fire();
    }

    self.impulse = 0;
}
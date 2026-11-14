// A fixture for test/pintool.c (S5): the file at the pin.
pub struct Score {
    value: u32,
}

impl Score {
    pub fn apply_penalty(&mut self, penalty: u32) {
        self.value = self.value.saturating_sub(penalty);
    }
}

pub const LIMIT: u32 = 1;

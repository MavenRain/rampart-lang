// A fixture for test/pintool.c (S5): a.rs after an upstream bump.
// New lines move each item down, a second type has a method with the
// same name, and two modules have a free fn with the same name.

pub struct Score {
    value: u32,
}

impl Other {
    pub fn apply_penalty(&mut self) {}
}

impl Score {
    pub fn apply_penalty(&mut self, penalty: u32) {
        self.value = self.value.saturating_sub(penalty);
    }
}

pub const LIMIT: u32 = 2;

mod one {
    pub fn helper() {}
}

mod two {
    pub fn helper() {}
}

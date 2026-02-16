# Tests Folder

Deze folder bevat alle test bestanden of tools voor het project.

1. **Test één ding:** Elke test test één specifieke functionaliteit
3. **Beschrijvende namen:** Test names moeten duidelijk zijn wat er getest wordt
4. **Independent tests:** Tests mogen niet van elkaar afhankelijk zijn
5. **Mock external dependencies:** Database calls, API calls, etc.

## Voorbeeld Test (pytest/Python)

```python
# tests/unit/test_math.py
import pytest
from src.utils.math import add, subtract

class TestMath:
    def test_add_positive_numbers(self):
        assert add(2, 3) == 5
    
    def test_add_negative_numbers(self):
        assert add(-2, 3) == 1
    
    def test_subtract(self):
        assert subtract(5, 3) == 2
```

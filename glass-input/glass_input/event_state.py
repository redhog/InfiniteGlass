import InfiniteGlass
import datetime
import operator

class EventStatePattern(InfiniteGlass.EventPattern):
    def __init__(self, pattern, display, state, config):
        self.state = state
        self.config = config
        pattern = pattern.split(",")
        self.filters = []
        self.statefilters = []
        self.parse_state_pattern(pattern)

        if not self.statefilters:
            self.state_filter = self.state_filter_none
        else:
            has_time = False
            for name, op, value in self.statefilters:
                cvalue = self.state.get(name, 0)
                if isinstance(cvalue, datetime.datetime):
                    has_time = True

            if has_time:
                self.state_filter = self.state_filter_time
            else:
                self.state_filter = self.state_filter_notime
        InfiniteGlass.EventPattern.__init__(self, self.filters, display)
        self.pattern = pattern

    def state_filter_time(self):
        now = datetime.datetime.now()
        for name, op, value in self.statefilters:
            cvalue = self.state.get(name, 0)
            if isinstance(cvalue, datetime.datetime):
                cvalue = (now - cvalue).total_seconds()
            if not op(cvalue, value):
                return False
        return True
    
    def state_filter_notime(self):
        for name, op, value in self.statefilters:
            cvalue = self.state.get(name, 0)
            if not op(cvalue, value):
                return False
        return True            

    def state_filter_none(self):
        return True

    def parse_state_pattern(self, items, negate=False):
        for item in items:
            if '==' in item:
                name, value = item.split('==')
                self.statefilters.append((name, operator.ne if negate else operator.eq, float(value)))
            elif '%' in item:
                name, value = item.split('%')
                self.statefilters.append((name, modulo, int(value)))
            elif '>=' in item:
                name, value = item.split('>=')
                self.statefilters.append((name, operator.lt if negate else operator.ge, float(value)))
            elif '<=' in item:
                name, value = item.split('<=')
                self.statefilters.append((name, operator.gt if negate else operator.le, float(value)))
            elif '>' in item:
                name, value = item.split('>')
                self.statefilters.append((name, operator.le if negate else operator.gt, float(value)))
            elif '<' in item:
                name, value = item.split('<')
                self.statefilters.append((name, operator.ge if negate else operator.lt, float(value)))
            elif item in self.config.get("eventfilters", {}):
                self.parse_state_pattern(self.config["eventfilters"][item].split(","), negate)
            elif item.startswith("!") and item[1:] in self.config.get("eventfilters", {}):
                self.parse_state_pattern(self.config["eventfilters"][item[1:]].split(","), not negate)
            else:
                if negate:
                    if item.startswith("!"):
                        item = item[1:]
                    else:
                        item = "!" + item
                self.filters.append(item)
    
    def equal(self, event):
        return self.state_filter() and InfiniteGlass.EventPattern.equal(self, event)

    def contained_by(self, event):
        return self.state_filter() and InfiniteGlass.EventPattern.contained_by(self, event)

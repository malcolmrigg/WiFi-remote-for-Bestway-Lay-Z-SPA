#include "CIO_6W.h"
#include "util.h"

void CIO_6W::setStates(const sToggles& requested_toggles)
{
    sButton_queue_item item;
    _handleButtonQ();
    if(_button_que_len > 30) return;
    /*Compare with previous requirement. Only add buttons to the queue at first change.*/
    /*If comparing with actual states it will add buttons to the queue every loop.*/ /*will try that anyway, with guards*/
    if(requested_toggles.power_change)
    {
        item.btncode = getButtonCode(POWER);
        item.p_state = &sStates::power;
        item.value = !_actual_states.power;
        item.duration_ms = 5000;
        _qButton(item);
        return;
    }

    if(requested_toggles.locked_change)
    {
        item.btncode = getButtonCode(LOCK);
        item.p_state = &sStates::locked;
        item.value = !_actual_states.locked;
        item.duration_ms = 5000;
        _qButton(item);
        return;
    }

    // if(requestedStates.unit != _requested_states.unit)
    if(requested_toggles.unit_change)
    {
        unlock();
        item.btncode = getButtonCode(UNIT);
        item.p_state = &sStates::unit;
        item.value = !_actual_states.unit;
        item.duration_ms = 5000;
        _qButton(item);

        // if((requestedStates.target >= 20) && (requestedStates.target <= 104) && (requestedStates.target == _requested_states.target))
        // {
        //     if(requestedStates.unit)
        //     {
        //         _actual_states.target = round(F2C(requestedStates.target));
        //     }
        //     else
        //     {
        //         _actual_states.target = round(C2F(requestedStates.target));
        //     }
        // }
    }

    // if((requestedStates.bubbles != _requested_states.bubbles) && getHasair())
    if(requested_toggles.bubbles_change && getHasair())
    {
        unlock();
        item.btncode = getButtonCode(BUBBLES);
        item.p_state = &sStates::bubbles;
        item.value = !_actual_states.bubbles;
        item.duration_ms = 5000;
        _qButton(item);
    }

    // if(requestedStates.heat != _requested_states.heat)
    if(requested_toggles.heat_change)
    {
        unlock();
        item.btncode = getButtonCode(HEAT);
        item.p_state = &sStates::heat;
        item.value = !_actual_states.heat;
        item.duration_ms = 5000;
        _qButton(item);
    }

    // if(requestedStates.pump != _requested_states.pump)
    if(requested_toggles.pump_change)
    {
        unlock();
        item.btncode = getButtonCode(PUMP);
        item.p_state = &sStates::pump;
        item.value = !_actual_states.pump;
        item.duration_ms = 5000;
        _qButton(item);
    }

    if(_actual_states.target == 0 && _button_que_len == 0)
    {
        unlock();
        item.btncode = getButtonCode(UP);
        item.p_state = &sStates::target;
        item.value = 999;
        item.duration_ms = 700;
        _qButton(item);
        item.btncode = getButtonCode(NOBTN);
        item.p_state = &sStates::char1;
        item.value = 0xFF;
        item.duration_ms = 500;
        _qButton(item);
    }

    if((requested_toggles.target != _actual_states.target))
    {
        unlock();
        Buttons dir;
        requested_toggles.target > _actual_states.target ? dir = UP : dir = DOWN;
        item.btncode = getButtonCode(dir);
        item.p_state = &sStates::target;
        item.value = requested_toggles.target;
        item.duration_ms = 1000;
        _qButton(item);
        item.btncode = getButtonCode(NOBTN);
        item.p_state = &sStates::char1;
        item.value = 0xFF;
        item.duration_ms = 500;
        _qButton(item);
    }

    // if((requestedStates.jets != _requested_states.jets) && getHasjets())
    if(requested_toggles.jets_change && getHasjets())
    {
        unlock();
        item.btncode = getButtonCode(HYDROJETS);
        item.p_state = &sStates::jets;
        item.value = !_actual_states.jets;
        item.duration_ms = 5000;
        _qButton(item);
    }

    _pressed_button = requested_toggles.pressed_button;
}

void CIO_6W::unlock()
{
    sButton_queue_item item;
    item.btncode = getButtonCode(POWER);
    item.p_state = &sStates::power;
    item.value = 1;
    item.duration_ms = 5000;
    _qButton(item);
    item.btncode = getButtonCode(LOCK);
    item.p_state = &sStates::locked;
    item.value = 0;
    item.duration_ms = 5000;
    _qButton(item);
}

void CIO_6W::_qButton(sButton_queue_item item) {
    if(_button_que_len >= MAXBUTTONS) return;  //maybe textout an error message if queue is full?
    _button_que[_button_que_len].btncode = item.btncode;
    _button_que[_button_que_len].p_state = item.p_state;
    _button_que[_button_que_len].value = item.value;
    _button_que[_button_que_len].duration_ms = item.duration_ms;
    _button_que_len++;
    /*"_button_que[0] = item" may not copy values but pointer to item which goes out of scope. Not sure.*/
}

void CIO_6W::_handleButtonQ(void) {
    static uint32_t prevMillis = millis();
    static uint32_t elapsedTime = 0;

    elapsedTime = millis() - prevMillis;
    prevMillis = millis();
    uint8_t waitlimit = 0;
    if(_button_que_len == 0)
    {
        /*Buttonqueue is empty, so let the touchbuttons from display/bwc through*/
        /*Avoiding write to variable when it's beeing sent to the cio*/
        waitlimit = 0;
        while(_packet_transm_active && ++waitlimit < 10) delay(1);
        _button_code = getButtonCode(NOBTN);
        if(_pressed_button == TIMER) _button_code = getButtonCode(TIMER);
        if(_pressed_button == UP) _button_code = getButtonCode(UP);
        if(_pressed_button == DOWN) _button_code = getButtonCode(DOWN);
        return;
    }
    // First subtract elapsed time from maxduration
    _button_que[0].duration_ms -= elapsedTime;
    //check if state is as desired, or duration is up. If so - remove row. Else set BTNCODE
    if( (_actual_states.*_button_que[0].p_state == _button_que[0].value) || (_button_que[0].duration_ms <= 0) )
    {
        //remove row
        for(int i = 0; i < _button_que_len-1; i++){
            _button_que[i].btncode = _button_que[i+1].btncode;
            _button_que[i].p_state = _button_que[i+1].p_state;
            _button_que[i].value = _button_que[i+1].value;
            _button_que[i].duration_ms = _button_que[i+1].duration_ms;
        }
        _button_que_len--;
        /*Avoiding write to variable when it's beeing sent to the cio*/
        waitlimit = 0;
        while(_packet_transm_active && ++waitlimit < 10) delay(1);
        _button_code = getButtonCode(NOBTN);
    }
    else
    {
        //keep button "pressed"
        /*Avoiding write to variable when it's beeing sent to the cio*/
        waitlimit = 0;
        while(_packet_transm_active && ++waitlimit < 10) delay(1);
        _button_code = _button_que[0].btncode;
    }
}

CIO_6W::CIO_6W()
{
    _actual_states.target = 0;
}

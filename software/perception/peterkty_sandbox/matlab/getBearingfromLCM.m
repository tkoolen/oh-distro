function [yk camposek] = getBearingAndCamposefromLCM()

lc = lcm.lcm.LCM.getSingleton();
aggregator = lcm.lcm.MessageAggregator();

lc.subscribe('OBJECT_BEARING', aggregator);

while true
    disp waiting_OBJECT_BEARING
    millis_to_wait = 1000;
    msg = aggregator.getNextMessage(millis_to_wait);
    if ~isempty(msg)
        break
    end
end

lc.unsubscribe('OBJECT_BEARING', aggregator);


lc.subscribe('BODY_TO_CAMERAHAND', aggregator);

while true
    disp waiting_BODY_TO_CAMERAHAND
    millis_to_wait = 1000;
    msg = aggregator.getNextMessage(millis_to_wait);
    if ~isempty(msg)
        break
    end
end

lc.unsubscribe('BODY_TO_CAMERAHAND', aggregator);
%lc.close();

bmsg = perception.pointing_vector_t(msg.data);
yk = atan([bmsg.vec(1), bmsg.vec(2)])';
fprintf('bearing angle(rad): %f %f', b(1), b(2));

end
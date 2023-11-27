namespace csm {

    typedef struct congestion_state {
        bool           send;     // all/limited
        unsigned short value;    // 0/num of pkt
        unsigned short delay;    // (optional) duration in nano or milli

        void update_state(bool s, unsigned short v, unsigned short d) {
            send  = s;
            value = v;
            delay = d;
        };

        void update_state(congestion_state *state) {
            send  = state->send;
            value = state->value;
            delay = state->delay;
        };
    } *CS;

    static CS state = new congestion_state { false, 0, 0 };

}    // namespace csm

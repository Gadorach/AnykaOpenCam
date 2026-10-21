#define GPIO0_HOST_TEST
#include "gpio0_observer.c"
#define CHECK(x) do { if(!(x)){fprintf(stderr,"fixture failure line %d\n",__LINE__);return 1;}cases++; } while(0)
int main(void) {
    unsigned cases=0,n=0;uint8_t a[GUARD+WIRE+GUARD];
    CHECK(sizeof(struct gpio_wire)==12);CHECK(offsetof(struct gpio_wire,value)==6);
    CHECK(GPIO_GET==0xc001662bUL);
    init_wire(a);CHECK(!reply_ok(a));a[GUARD+6]=0;CHECK(reply_ok(a));a[GUARD+6]=1;CHECK(reply_ok(a));
    a[GUARD+6]=2;CHECK(!reply_ok(a));a[GUARD+6]=1;
    for(unsigned i=0;i<sizeof(a);++i)if(i!=GUARD+6){a[i]^=1;CHECK(!reply_ok(a));a[i]^=1;}
    struct snap s={0,0,0,0,1},t=s;CHECK(input_ok(s));t.mux=1;CHECK(!input_ok(t));CHECK(!config_equal(s,t));
    t=s;t.dir=1;CHECK(!input_ok(t));CHECK(!config_equal(s,t));t=s;t.out=1;CHECK(!config_equal(s,t));
    t=s;t.pull_disabled=1;CHECK(!config_equal(s,t));t=s;t.level=0;CHECK(config_equal(s,t));
    CHECK(parse_ms("1000",&n)&&n==1000);CHECK(parse_ms("60000",&n)&&n==60000);
    const char *bad[]={"0","999","60001","-1","+1000","1.5","1000x","","999999999999"};
    for(unsigned i=0;i<sizeof(bad)/sizeof(bad[0]);++i)CHECK(!parse_ms(bad[i],&n));
    printf("{\"host_cases\":%u,\"hardware_access\":false,\"pure_wire_config_and_argument_fixtures\":true}\n",cases);
    return 0;
}

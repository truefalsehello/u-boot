#include <common.h>
#include <g_dnl.h>
#include <linux/usb/composite.h>

#define MINE_DRV_NAME "Mine Function"
#define MINE_STRING_INTERFACE 0

#define USB_REQUEST_GET_DESCRIPTOR 0x06
#define USB_DT_HID 0x21
#define USB_DT_REPORT 0x22

static void mine_interrupt_complete(struct usb_ep *ep,
                                    struct usb_request *req);

static struct mine_t
{
    struct usb_function func;
    struct usb_ep *ep_interrupt;
    struct usb_request *ep_interrupt_req;

    struct usb_ep *ep0;         /* Copy of gadget->ep0 */
    struct usb_request *ep0req; /* Copy of cdev->req */
} mine;

struct hid_descriptor
{
    u8 bLength;
    u8 bDescriptorType;
    u16 bcdHID;
    u8 bCountryCode;
    u8 bNumDescriptors;
    u8 bReportDescriptorType;
    u16 wDescriptorLength;
} __attribute__((packed));

static int a_count = 0;

static u8 down_a[] = {
    0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00};

static u8 up_a[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static void mine_interrupt_complete(struct usb_ep *ep,
                                    struct usb_request *req)
{
    printf("%s\n",__func__);
    if((a_count++) < 15)
        usb_ep_queue(mine.ep_interrupt, mine.ep_interrupt_req, GFP_ATOMIC);
};

static struct usb_string mine_strings[] = {
    {MINE_STRING_INTERFACE, "Mine keyboard"},
    {}};

static struct usb_gadget_strings mine_stringtab_en = {
    .language = 0x0409,
    .strings = mine_strings};

static struct usb_gadget_strings *mine_strings_array[] = {
    &mine_stringtab_en,
    NULL};

static struct usb_interface_descriptor mine_intf_desc = {
    .bLength = sizeof(mine_intf_desc),
    .bDescriptorType = USB_DT_INTERFACE,
    .bNumEndpoints = 1,
    .bInterfaceClass = USB_CLASS_HID,
    .bInterfaceSubClass = 0x01,
    .bInterfaceProtocol = 0x01,
    .iInterface = MINE_STRING_INTERFACE};

static struct usb_endpoint_descriptor mine_ep_desc = {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = USB_DIR_IN,
    .bmAttributes = USB_ENDPOINT_XFER_INT,
    .wMaxPacketSize = 8,
    .bInterval = 0x0a};

static u8 keyboard_report_descriptor[] = {
    0x05, 0x01, 0x09, 0x06, 0xA1, 0x01, 0x05, 0x07, 0x19, 0xE0, 0x29, 0xE7, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x08, 0x81, 0x02, 0x95, 0x01, 0x75, 0x08, 0x81, 0x01, 0x95, 0x03, 0x75, 0x01, 0x05, 0x08, 0x19, 0x01, 0x29, 0x03, 0x91, 0x02, 0x95, 0x01, 0x75, 0x05, 0x91, 0x01, 0x95, 0x06, 0x75, 0x08, 0x15, 0x00, 0x26, 0xFF, 0x00, 0x05, 0x07, 0x19, 0x00, 0x2A, 0xFF, 0x00, 0x81, 0x00, 0xC0};

static struct hid_descriptor mine_report_desc = {
    .bLength = sizeof(struct hid_descriptor),
    .bDescriptorType = USB_DT_HID,
    .bcdHID = 0x0111,     // HID version 1.11
    .bCountryCode = 0x00, // No localization
    .bNumDescriptors = 0x01,
    .bReportDescriptorType = USB_DT_REPORT,
    .wDescriptorLength = sizeof(keyboard_report_descriptor),
};

static struct usb_descriptor_header *mine_function[] = {
    (struct usb_descriptor_header *)&mine_intf_desc,
    (struct usb_descriptor_header *)&mine_report_desc,
    (struct usb_descriptor_header *)&mine_ep_desc,
    NULL};

static struct usb_descriptor_header **
usb_copy_descriptors(struct usb_descriptor_header **src)
{
    struct usb_descriptor_header **tmp;
    unsigned bytes;
    unsigned n_desc;
    void *mem;
    struct usb_descriptor_header **ret;

    /* count descriptors and their sizes; then add vector size */
    for (bytes = 0, n_desc = 0, tmp = src; *tmp; tmp++, n_desc++)
        bytes += (*tmp)->bLength;
    bytes += (n_desc + 1) * sizeof(*tmp);

    mem = memalign(CONFIG_SYS_CACHELINE_SIZE, bytes);
    if (!mem)
        return NULL;

    /* fill in pointers starting at "tmp",
     * to descriptors copied starting at "mem";
     * and return "ret"
     */
    tmp = mem;
    ret = mem;
    mem += (n_desc + 1) * sizeof(*tmp);
    while (*src)
    {
        memcpy(mem, *src, (*src)->bLength);
        *tmp = mem;
        tmp++;
        mem += (*src)->bLength;
        src++;
    }
    *tmp = NULL;

    return ret;
}

static void mine_disable(struct usb_function *f)
{
}

static int mine_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
    int rc;
    mine.ep_interrupt_req = usb_ep_alloc_request(mine.ep_interrupt, GFP_ATOMIC);
    if (mine.ep_interrupt_req == NULL)
    {
        printf("can't allocate request\n");
        return 0;
    }
    mine.ep_interrupt_req->buf = down_a;
    mine.ep_interrupt_req->length = sizeof(down_a);
    mine.ep_interrupt_req->complete = mine_interrupt_complete;
    rc = usb_ep_enable(mine.ep_interrupt, &mine_ep_desc);
    if (rc){
        printf("can't enable %s, result %d\n", mine.ep_interrupt->name, rc);
        return 0;
    }
    usb_ep_queue(mine.ep_interrupt, mine.ep_interrupt_req, GFP_ATOMIC);
    return 0;
}

static int mine_setup(struct usb_function *f,
                      const struct usb_ctrlrequest *ctrl)
{
    int rc = 0;

    switch (ctrl->bRequest)
    {
    case USB_REQUEST_GET_DESCRIPTOR:
        switch (ctrl->wValue >> 8)
        {
        case USB_DT_REPORT:
            memcpy(mine.ep0req->buf, keyboard_report_descriptor, sizeof(keyboard_report_descriptor));
            mine.ep0req->length = sizeof(keyboard_report_descriptor);
            rc = usb_ep_queue(mine.ep0, mine.ep0req, GFP_ATOMIC);
            return rc;
        }
        break;
    }
    return -EOPNOTSUPP;
}

static int mine_bind(struct usb_configuration *c, struct usb_function *f)
{
    int inf_id;
    struct usb_ep *ep;

    printf("%s\n", __func__);
    inf_id = usb_interface_id(c, f);
    if (inf_id < 0)
    {
        return inf_id;
    }
    mine_intf_desc.bInterfaceNumber = inf_id;

    ep = usb_ep_autoconfig(c->cdev->gadget, &mine_ep_desc);
    if (!ep)
        goto autoconf_fail;

    ep->driver_data = &mine;
    mine.ep_interrupt = ep;

    f->descriptors = usb_copy_descriptors(mine_function);
    if (unlikely(!f->descriptors))
        return -ENOMEM;

    return 0;

autoconf_fail:
    error("unable to autoconfigure all endpoints\n");
    return -ENOTSUPP;
}

static void mine_unbind(struct usb_configuration *c, struct usb_function *f)
{
    printf("%s\n", __func__);
    usb_ep_free_request(mine.ep_interrupt, mine.ep_interrupt_req);
    free(mine.func.descriptors);
}

static int mine_add(struct usb_configuration *c)
{
    int rc;
    struct usb_device_descriptor *dev;
    struct usb_string *strings;

    printf("%s\n", __func__);

    mine.ep0 = c->cdev->gadget->ep0;
    mine.ep0req = c->cdev->req;

    dev = c->cdev->driver->dev;
    dev->bcdUSB = 0x0110;
    dev->bDeviceClass = 0;
    dev->bDeviceSubClass = 0;

    strings = c->cdev->driver->strings[0]->strings;
    strings[0].s = "My company";
    strings[1].s = "Example keyboard";

    g_dnl_set_serialnumber("9876543210987654321098765432101");

    if (mine_strings[MINE_STRING_INTERFACE].id == 0)
    {
        rc = usb_string_id(c->cdev);
        if (unlikely(rc < 0))
            return ERR_PTR(rc);
        mine_strings[MINE_STRING_INTERFACE].id = rc;
        mine_intf_desc.iInterface = rc;
    }

    mine.func.name = MINE_DRV_NAME;
    mine.func.strings = mine_strings_array;
    mine.func.bind = mine_bind;
    mine.func.unbind = mine_unbind;
    mine.func.setup = mine_setup;
    mine.func.set_alt = mine_set_alt;
    mine.func.disable = mine_disable;

    return usb_add_function(c, &mine.func);
}

DECLARE_GADGET_BIND_CALLBACK(usb_mine_k, mine_add);

import { mdiClockOutline, mdiContentSave, mdiMapMarkerRadius } from "@mdi/js";
import { type Component, createSignal, For } from "solid-js";

import { Icon } from "../components/Icon";
import { Toast } from "../components/Toast";
import { TIME_FORMAT, TIME_POSIX, TIME_ZONE } from "../constants/secrets";
import { ZONES } from "../constants/zones";
import { WebSocketWS } from "../extensions/WebSocket";

export const name = "Time";

const browserZone = Intl.DateTimeFormat().resolvedOptions().timeZone;

const [getPosix, setPosix] = createSignal<string>(TIME_POSIX);
const [getZone, setZone] = createSignal<string>(TIME_ZONE);

const [getFormFormat, setFormFormat] = createSignal<number>(TIME_FORMAT);
const [getFormServer, setFormServer] = createSignal<string>("");
const [getFormZone, setFormZone] = createSignal<string>(TIME_ZONE);

/**
 * Updates the time zone, NTP server and clock format from a time payload, and resets the form to them.
 * Omitted or undefined fields leave the corresponding state unchanged.
 */
export const receiver = (json: {
  format?: number;
  posix?: string;
  server?: string;
  zone?: string;
}) => {
  if (json?.format !== undefined) {
    setFormFormat(json.format);
  }
  if (json?.posix !== undefined) {
    setPosix(json.posix);
  }
  if (json?.server !== undefined) {
    setFormServer(json.server);
  }
  if (json?.zone !== undefined) {
    setZone(json.zone);
    setFormZone(json.zone);
  }
};

const { toast } = Toast();

/**
 * Renders the time zone, NTP server and clock format settings.
 * Enables saving once a known time zone is entered.
 */
export const MainThird: Component = () => {
  const handleSave = () => {
    WebSocketWS.send(
      JSON.stringify({
        [name]: {
          format: getFormFormat(),
          posix: ZONES[getFormZone()],
          server: getFormServer().trim(),
          zone: getFormZone(),
        },
      }),
    );
    toast("Saved");
  };

  return (
    <div class="main">
      <div class="space-y-3 p-5">
        <h2>{name}</h2>
        <div class="box">
          <div class="space-y-2">
            {getZone() in ZONES && ZONES[getZone()] !== getPosix() && (
              <div class="text-sm">Time zone rules have been updated, save to apply.</div>
            )}
            <div class="grid gap-3">
              <h3>Time zone</h3>
              <input
                class="w-full"
                id="zone"
                list="zones"
                oninput={(e) => setFormZone(e.currentTarget.value)}
                placeholder={getZone() || "Region/City"}
                type="text"
                value={getFormZone()}
              />
              <datalist id="zones">
                <For each={Object.keys(ZONES)}>{(zone) => <option value={zone}></option>}</For>
              </datalist>
              {browserZone in ZONES && browserZone !== getFormZone() && (
                <button
                  class="action-neutral w-full"
                  onclick={() => setFormZone(browserZone)}
                  type="button"
                >
                  <Icon class="mr-2" path={mdiMapMarkerRadius} />
                  Use {browserZone}
                </button>
              )}
              <h3>NTP server</h3>
              <input
                class="w-full"
                id="server"
                oninput={(e) => setFormServer(e.currentTarget.value)}
                placeholder="pool.ntp.org"
                type="text"
                value={getFormServer()}
              />
              <h3>Clock format</h3>
              <select
                class="w-full"
                onchange={(e) => setFormFormat(Number(e.currentTarget.value))}
                value={getFormFormat()}
              >
                <option selected={getFormFormat() === 24} value={24}>
                  24-hour
                </option>
                <option selected={getFormFormat() === 12} value={12}>
                  12-hour
                </option>
              </select>
            </div>
            <button
              class="action-positive mt-3 w-full"
              disabled={!(getFormZone() in ZONES)}
              id="save"
              onclick={handleSave}
              type="button"
            >
              <Icon class="mr-2" path={mdiContentSave} />
              Save
            </button>
          </div>
        </div>
      </div>
    </div>
  );
};

export const SidebarSecondaryComponent: Component = () => (
  <a class="link" href={`#/${name.toLowerCase()}`}>
    <Icon class="mr-2" path={mdiClockOutline} />
    {getZone() || name}
  </a>
);

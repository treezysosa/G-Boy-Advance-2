#!/usr/bin/env python3
"""Exercise actual offline FLAC playback and feedback without human verdicts."""
import argparse
import json
from pathlib import Path
import shutil
from playwright.sync_api import sync_playwright

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('pack', type=Path)
parser.add_argument('--browser')
args = parser.parse_args()
pack = args.pack.resolve()
data = json.loads((pack/'manifest.json').read_text())
assert len(data['presets']) >= 3
with sync_playwright() as playwright:
    executable = args.browser or shutil.which('google-chrome') or shutil.which('chromium')
    browser = playwright.chromium.launch(headless=True, executable_path=executable, timeout=20000,
                                         args=['--no-sandbox', '--disable-dev-shm-usage'])
    context = browser.new_context(accept_downloads=True, viewport={'width': 1280, 'height': 1000})
    page = context.new_page()
    page.set_default_timeout(15000)
    errors = []
    page.on('pageerror', lambda error: errors.append(str(error)))
    page.goto((pack/'Listen.html').as_uri())
    assert page.locator('#list button').count() == len(data['presets'])
    assert page.locator('#progressText').inner_text().startswith('0 /')
    assert page.evaluate('document.createElement("audio").canPlayType("audio/flac")')
    page.locator('#playA').click()
    page.wait_for_function('!document.querySelector("#player").paused && document.querySelector("#player").currentTime > 0')
    assert page.evaluate('document.querySelector("#player").duration') == 20
    page.locator('#playB').click()
    page.wait_for_function('document.querySelector("#playing").textContent.startsWith("B ·") && !document.querySelector("#player").paused')
    assert 0 < page.evaluate('document.querySelector("#player").volume') <= .7
    page.locator('#stop').click()
    assert page.evaluate('document.querySelector("#player").paused && document.querySelector("#player").currentTime===0')
    page.locator('#verdict').select_option('too-quiet')
    page.locator('#notes').fill('AUTOMATED FIXTURE — not a listening judgment')
    page.locator('#trim').fill('1.2')
    page.locator('#save').click()
    assert page.locator('#progressText').inner_text().startswith('1 /')
    with page.expect_download() as download:
        page.locator('#export').click()
    path = pack/'test-feedback.json'
    download.value.save_as(path)
    feedback = json.loads(path.read_text())
    first = data['presets'][0]['id']
    assert feedback['reviews'][first]['gainDb'] == 1.2
    assert feedback['reviews'][first]['notes'].startswith('AUTOMATED FIXTURE')
    page.reload()
    assert page.locator('#notes').input_value().startswith('AUTOMATED FIXTURE')
    page.locator('#filter').select_option('reviewed')
    assert page.locator('#list button').count() == 1
    page.locator('#filter').select_option('unreviewed')
    assert page.locator('#list button').count() == len(data['presets'])-1
    page.locator('#filter').select_option('all')
    page.locator('#search').fill(data['presets'][0]['name'])
    assert page.locator('#list button').count() >= 1
    page.locator('#search').fill('deliberately absent preset name')
    assert page.locator('#list button').count() == 0
    page.locator('#search').fill('')
    page.evaluate('localStorage.clear()')
    page.reload()
    assert page.locator('#progressText').inner_text().startswith('0 /')
    page.locator('#feedbackFile').set_input_files(path)
    page.wait_for_function('document.querySelector("#saved").textContent==="Feedback imported."')
    assert page.locator('#progressText').inner_text().startswith('1 /')
    bad = dict(feedback, factorySha256='wrong-bank')
    bad_path = pack/'test-wrong-bank.json'
    bad_path.write_text(json.dumps(bad))
    page.locator('#feedbackFile').set_input_files(bad_path)
    page.wait_for_function('document.querySelector("#error").textContent.includes("different bank")')
    assert page.locator('#progressText').inner_text().startswith('1 /')
    # Remove all fixture feedback; no test can mark factory sounds human-approved.
    page.evaluate('localStorage.clear()')
    page.reload()
    page.screenshot(path=str(pack/'review-player.png'), full_page=True)
    assert not errors, errors
    assert page.locator('#progressText').inner_text().startswith('0 /')
    context.close()
    browser.close()
    path.unlink()
    bad_path.unlink()
print('PASS offline FLAC A/B playback, attenuation, search, review filters, save/reload, export/import and wrong-bank rejection; all test verdicts removed')

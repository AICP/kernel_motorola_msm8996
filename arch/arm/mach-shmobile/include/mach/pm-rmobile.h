/*
 * Copyright (C) 2012 Renesas Solutions Corp.
 *
 * Kuninori Morimoto <morimoto.kuninori@renesas.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */
#ifndef PM_RMOBILE_H
#define PM_RMOBILE_H

#include <linux/pm_domain.h>

struct platform_device;

struct rmobile_pm_domain {
	struct generic_pm_domain genpd;
	struct dev_power_governor *gov;
	int (*suspend)(void);
	void (*resume)(void);
	unsigned int bit_shift;
	bool no_debug;
};

static inline
struct rmobile_pm_domain *to_rmobile_pd(struct generic_pm_domain *d)
{
	return container_of(d, struct rmobile_pm_domain, genpd);
}

#ifdef CONFIG_PM
extern void rmobile_init_domains(struct rmobile_pm_domain domains[], int num);
extern void rmobile_add_device_to_domain_td(const char *domain_name,
					    struct platform_device *pdev,
					    struct gpd_timing_data *td);

static inline void rmobile_add_device_to_domain(const char *domain_name,
						struct platform_device *pdev)
{
	rmobile_add_device_to_domain_td(domain_name, pdev, NULL);
}

#else

#define rmobile_init_domains(domains, num) do { } while (0)
#define rmobile_add_device_to_domain_td(name, pdev, td) do { } while (0)
#define rmobile_add_device_to_domain(name, pdev) do { } while (0)
#endif /* CONFIG_PM */

#endif /* PM_RMOBILE_H */
